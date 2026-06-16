//
// 完整 SQL CRUD 端到端测试：通过 SQL 字符串走 parser -> binder -> planner -> executor
//
#include <cstdio>

#include "gtest/gtest.h"

#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "executor/executor_delete.h"
#include "executor/executor_update.h"
#include "parser/parser.h"
#include "parser/statement/delete_statement.h"
#include "parser/statement/insert_statement.h"
#include "parser/statement/select_statement.h"
#include "parser/statement/update_statement.h"
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

// 把一条 SQL 走完整链路并执行（每条语句独立事务）
auto RunSQL(Db &db, const std::string &sql) -> size_t {
    Parser parser;
    parser.ParserSQL(sql);
    auto &raw = parser.GetStatements()[0];

    Binder binder(db.catalog);
    std::unique_ptr<BoundStatement> bound;

    if (auto *s = dynamic_cast<SelectStatement *>(raw.get())) {
        raw.release();
        bound = binder.BoundSelectStatement(std::unique_ptr<SelectStatement>(s));
    } else if (auto *s = dynamic_cast<InsertStatement *>(raw.get())) {
        raw.release();
        bound = binder.BoundInsertStatement(std::unique_ptr<InsertStatement>(s));
    } else if (auto *s = dynamic_cast<DeleteStatement *>(raw.get())) {
        raw.release();
        bound = binder.BoundDeleteStatement(std::unique_ptr<DeleteStatement>(s));
    } else if (auto *s = dynamic_cast<UpdateStatement *>(raw.get())) {
        raw.release();
        bound = binder.BoundUpdateStatement(std::unique_ptr<UpdateStatement>(s));
    } else {
        throw std::runtime_error("unsupported statement in RunSQL");
    }

    auto type = bound->type_;
    Planner planner;
    auto logical = planner.CreateLogicalPlan(std::move(bound));
    auto physical = planner.CreatePhysicalPlan(logical);

    auto txn = db.txn_mgr->Begin();
    auto ctx = std::make_shared<ExecutorContext>(db.catalog, db.buffer, db.meta,
                                                 db.txn_mgr, txn, db.undo_mgr);
    ExecutorFactory factory(ctx);
    auto exec = factory.BuildExecutor(physical);

    size_t rows = 0;
    Execute execute(ctx);
    if (type == StatementType::SELECT) {
        exec->Open();
        Tuple t;
        while (exec->Next(&t)) ++rows;
        exec->Close();
    } else if (type == StatementType::INSERT) {
        execute.ExecuteInsert(std::move(exec));
    } else if (type == StatementType::DELETE) {
        exec->Open();
        Tuple t;
        while (exec->Next(&t)) {}
        if (auto *d = dynamic_cast<ExecutorDelete *>(exec.get())) rows = d->GetDeletedCount();
        exec->Close();
    } else if (type == StatementType::UPDATE) {
        exec->Open();
        Tuple t;
        while (exec->Next(&t)) {}
        if (auto *u = dynamic_cast<ExecutorUpdate *>(exec.get())) rows = u->GetUpdatedCount();
        exec->Close();
    }
    db.txn_mgr->Commit(txn->tx_id_);
    return rows;
}

// 统计某表当前可见行数（SELECT *）
auto VisibleCount(Db &db, const std::string &table) -> size_t {
    return RunSQL(db, "select * from " + table);
}

auto MakeDb() -> Db {
    Db db;
    db.disk = std::make_shared<PosixDiskManager>();
    db.buffer = std::make_shared<BufferManager>(db.disk);
    db.meta = std::make_shared<MetaPage>(db.buffer);
    db.txn_mgr = std::make_shared<TransactionManager>();
    db.undo_mgr = std::make_shared<UndoLogManager>(db.buffer);
    db.catalog = std::make_shared<Catalog>();
    return db;
}

// 手动登记一张表到 catalog + meta（绕过 CREATE TABLE 落盘细节，聚焦 DML）
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

}  // namespace

TEST(CrudSQL, InsertSelectDeleteUpdate) {
    // 用独立表名隔离磁盘状态
    Db db = MakeDb();
    std::string tbl = "crud_t" + std::to_string(IdManager::GetNextEntryId());
    RegisterTable(db, tbl);

    // INSERT 5 行
    for (int i = 0; i < 5; ++i) {
        RunSQL(db, "insert into " + tbl + " (name,age) values('u" + std::to_string(i) + "'," +
                       std::to_string(10 + i) + ")");
    }
    EXPECT_EQ(VisibleCount(db, tbl), 5u);

    // DELETE WHERE age = 12  -> 删 1 行
    size_t deleted = RunSQL(db, "delete from " + tbl + " where age = 12");
    EXPECT_EQ(deleted, 1u);
    EXPECT_EQ(VisibleCount(db, tbl), 4u);

    // UPDATE WHERE age = 10 SET age = 99 -> 改 1 行（delete-old + insert-new）
    size_t updated = RunSQL(db, "update " + tbl + " set age = 99 where age = 10");
    EXPECT_EQ(updated, 1u);
    // 行数不变：旧版本被标记删除，新版本插入，可见仍为 4
    EXPECT_EQ(VisibleCount(db, tbl), 4u);

    // 更新后 age=99 应可被再次命中删除
    size_t deleted2 = RunSQL(db, "delete from " + tbl + " where age = 99");
    EXPECT_EQ(deleted2, 1u);
    EXPECT_EQ(VisibleCount(db, tbl), 3u);
}
