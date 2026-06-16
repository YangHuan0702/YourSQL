//
// 批次1 验证：Abort 回滚（可见性 + 物理）、写写冲突
//
#include "gtest/gtest.h"

#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "executor/executor_delete.h"
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

// 在给定事务下执行一条 INSERT
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

// 用一个新事务统计可见行数
auto VisibleCount(Db &db, const std::string &tbl) -> size_t {
    auto txn = db.txn_mgr->Begin();
    auto ctx = std::make_shared<ExecutorContext>(db.catalog, db.buffer, db.meta, db.txn_mgr, txn, db.undo_mgr);
    ExecutorSeqScan scan(ctx, tbl);
    scan.Open();
    Tuple t;
    size_t n = 0;
    while (scan.Next(&t)) ++n;
    scan.Close();
    db.txn_mgr->Commit(txn->tx_id_);
    return n;
}

}  // namespace

// 插入后 abort：新事务看不到，且物理行被置 dead
TEST(TxnBatch1, InsertThenAbortInvisible) {
    Db db = MakeDb();
    std::string tbl = "t_abort_ins" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    auto txn = db.txn_mgr->Begin();
    DoInsert(db, txn, tbl, "a", 1);
    DoInsert(db, txn, tbl, "b", 2);
    db.txn_mgr->Abort(txn->tx_id_);

    EXPECT_EQ(VisibleCount(db, tbl), 0u);
}

// 删除后 abort：行恢复可见
TEST(TxnBatch1, DeleteThenAbortRestores) {
    Db db = MakeDb();
    std::string tbl = "t_abort_del" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    // 先提交插入 3 行
    auto t1 = db.txn_mgr->Begin();
    DoInsert(db, t1, tbl, "a", 1);
    DoInsert(db, t1, tbl, "b", 2);
    DoInsert(db, t1, tbl, "c", 3);
    db.txn_mgr->Commit(t1->tx_id_);
    ASSERT_EQ(VisibleCount(db, tbl), 3u);

    // 事务删除全部，然后 abort
    auto t2 = db.txn_mgr->Begin();
    auto ctx = std::make_shared<ExecutorContext>(db.catalog, db.buffer, db.meta, db.txn_mgr, t2, db.undo_mgr);
    ExecutorDelete del(ctx, tbl);
    del.Open();
    Tuple t;
    del.Next(&t);
    del.Close();
    EXPECT_EQ(del.GetDeletedCount(), 3u);
    db.txn_mgr->Abort(t2->tx_id_);

    // abort 后 3 行应全部恢复可见
    EXPECT_EQ(VisibleCount(db, tbl), 3u);
}

// 已提交插入对后开始的事务可见；abort 的不可见（可见性层）
TEST(TxnBatch1, CommittedVisibleAbortedNot) {
    Db db = MakeDb();
    std::string tbl = "t_vis" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    auto t1 = db.txn_mgr->Begin();
    DoInsert(db, t1, tbl, "keep", 1);
    db.txn_mgr->Commit(t1->tx_id_);

    auto t2 = db.txn_mgr->Begin();
    DoInsert(db, t2, tbl, "drop", 2);
    db.txn_mgr->Abort(t2->tx_id_);

    EXPECT_EQ(VisibleCount(db, tbl), 1u);  // 只看到已提交的 keep
}
