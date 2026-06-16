//
// 批次2 验证：事务跨语句、RR vs RC 快照、ROLLBACK
//
#include "gtest/gtest.h"

#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "executor/executor_seq_scan.h"
#include "parser/parser.h"
#include "parser/statement/insert_statement.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"
#include "transaction/transaction_manager.h"
#include "transaction/undo_log_manager.h"

using namespace YourSQL;

namespace {

struct Db {
    std::shared_ptr<Catalog> catalog;
    std::shared_ptr<PosixDiskManager> disk;
    std::shared_ptr<BufferManager> buffer;
    std::shared_ptr<MetaPage> meta;
    std::shared_ptr<TransactionManager> txn_mgr;
    std::shared_ptr<UndoLogManager> undo_mgr;
};

auto MakeDb() -> Db {
    Db db;
    db.disk = std::make_shared<PosixDiskManager>();
    db.buffer = std::make_shared<BufferManager>(db.disk);
    db.meta = std::make_shared<MetaPage>(db.buffer);
    db.txn_mgr = std::make_shared<TransactionManager>();
    db.txn_mgr->SetStorage(db.buffer.get(), db.meta.get());
    db.undo_mgr = std::make_shared<UndoLogManager>(db.buffer);
    db.catalog = std::make_shared<Catalog>();
    return db;
}

auto RegisterTable(Db &db, const std::string &table_name) -> entry_id {
    std::string name_copy = table_name;
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), name_copy);
    entry_id table_id = table->id_;
    std::string c_name = "name", c_age = "age";
    table->AddColumn(ColumnEntry(table->GetNextColumnId(), c_name, ColumnTypes::VARCHAR));
    table->AddColumn(ColumnEntry(table->GetNextColumnId(), c_age, ColumnTypes::INTEGER));
    db.catalog->AddTable(std::move(table));
    if (db.meta->items_.find(table_id) == db.meta->items_.end()) {
        MetaItem item;
        item.table_id_ = table_id;
        item.table_name_ = table_name;
        item.first_page_id = 0;
        item.last_page_id = 0;
        item.num_rows_ = 0;
        item.items_.push_back(MetaColumnItem{c_name, ColumnTypes::VARCHAR, 0, 0});
        item.items_.push_back(MetaColumnItem{c_age, ColumnTypes::INTEGER, 0, 1});
        db.meta->AddTable(item);
    }
    return table_id;
}

auto DoInsert(Db &db, const std::shared_ptr<Transaction> &txn, const std::string &tbl,
              const std::string &name, int age) -> void {
    Parser parser;
    parser.ParserSQL("insert into " + tbl + " (name,age) values('" + name + "'," + std::to_string(age) + ")");
    auto &raw = parser.GetStatements()[0];
    raw.release();
    Binder binder(db.catalog);
    auto bound = binder.BoundInsertStatement(
        std::unique_ptr<InsertStatement>(dynamic_cast<InsertStatement *>(raw.get())));
    Planner planner;
    auto logical = planner.CreateLogicalPlan(std::move(bound));
    auto physical = planner.CreatePhysicalPlan(logical);
    auto ctx = std::make_shared<ExecutorContext>(db.catalog, db.buffer, db.meta, db.txn_mgr, txn, db.undo_mgr);
    ExecutorFactory factory(ctx);
    auto exec = factory.BuildExecutor(physical);
    Execute execute(ctx);
    execute.ExecuteInsert(std::move(exec));
}

// 在给定事务下扫描可见行数（复用该事务的快照）
auto CountUnder(Db &db, const std::shared_ptr<Transaction> &txn, const std::string &tbl) -> size_t {
    auto ctx = std::make_shared<ExecutorContext>(db.catalog, db.buffer, db.meta, db.txn_mgr, txn, db.undo_mgr);
    ExecutorSeqScan scan(ctx, tbl);
    scan.Open();
    Tuple t;
    size_t n = 0;
    while (scan.Next(&t)) ++n;
    scan.Close();
    return n;
}

}  // namespace

// REPEATABLE READ：事务内两次读结果一致，即使期间另一事务提交了插入
TEST(TxnBatch2, RepeatableReadStableSnapshot) {
    Db db = MakeDb();
    std::string tbl = "t_rr" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    auto seed = db.txn_mgr->Begin();
    DoInsert(db, seed, tbl, "a", 1);
    db.txn_mgr->Commit(seed->tx_id_);

    // 读事务（RR）开始，拍快照
    auto reader = db.txn_mgr->Begin();
    reader->isolation_level_ = IsolationLevel::REPEATABLE_READ;
    size_t first = CountUnder(db, reader, tbl);
    EXPECT_EQ(first, 1u);

    // 另一事务提交插入
    auto writer = db.txn_mgr->Begin();
    DoInsert(db, writer, tbl, "b", 2);
    db.txn_mgr->Commit(writer->tx_id_);

    // RR：reader 复用旧快照，仍只看到 1 行
    size_t second = CountUnder(db, reader, tbl);
    EXPECT_EQ(second, 1u);
    db.txn_mgr->Commit(reader->tx_id_);

    // 新事务能看到 2 行
    auto after = db.txn_mgr->Begin();
    EXPECT_EQ(CountUnder(db, after, tbl), 2u);
    db.txn_mgr->Commit(after->tx_id_);
}

// READ COMMITTED：每条语句前刷新快照，能看到期间提交的插入
TEST(TxnBatch2, ReadCommittedSeesNewCommits) {
    Db db = MakeDb();
    std::string tbl = "t_rc" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    auto seed = db.txn_mgr->Begin();
    DoInsert(db, seed, tbl, "a", 1);
    db.txn_mgr->Commit(seed->tx_id_);

    auto reader = db.txn_mgr->Begin();
    reader->isolation_level_ = IsolationLevel::READ_COMMITTED;
    EXPECT_EQ(CountUnder(db, reader, tbl), 1u);

    auto writer = db.txn_mgr->Begin();
    DoInsert(db, writer, tbl, "b", 2);
    db.txn_mgr->Commit(writer->tx_id_);

    // RC：刷新快照后再读，能看到新提交的行
    reader->read_view_ = db.txn_mgr->CreateReadView(reader->tx_id_);
    EXPECT_EQ(CountUnder(db, reader, tbl), 2u);
    db.txn_mgr->Commit(reader->tx_id_);
}

// 显式事务 ROLLBACK 后改动消失
TEST(TxnBatch2, RollbackDiscardsChanges) {
    Db db = MakeDb();
    std::string tbl = "t_rb" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    auto txn = db.txn_mgr->Begin();
    DoInsert(db, txn, tbl, "x", 1);
    DoInsert(db, txn, tbl, "y", 2);
    db.txn_mgr->Abort(txn->tx_id_);  // ROLLBACK

    auto after = db.txn_mgr->Begin();
    EXPECT_EQ(CountUnder(db, after, tbl), 0u);
    db.txn_mgr->Commit(after->tx_id_);
}
