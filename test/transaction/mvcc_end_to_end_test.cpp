//
// MVCC 端到端验证：insert(多行/翻页) -> seq scan 可见性 -> delete -> 再 scan
//
#include "gtest/gtest.h"

#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "executor/executor_delete.h"
#include "parser/parser.h"
#include "parser/statement/insert_statement.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"
#include "transaction/transaction_manager.h"
#include "transaction/undo_log_manager.h"

using namespace YourSQL;

namespace {

// 统计一次 SELECT * 的可见行数
auto CountVisibleRows(const std::shared_ptr<Catalog> &catalog,
                      const std::shared_ptr<BufferManager> &buffer_manager,
                      const std::shared_ptr<MetaPage> &meta_page,
                      const std::shared_ptr<TransactionManager> &txn_mgr,
                      const std::shared_ptr<UndoLogManager> &undo_mgr,
                      const std::string &table_name) -> size_t {
    auto txn = txn_mgr->Begin();
    auto ctx = std::make_shared<ExecutorContext>(catalog, buffer_manager, meta_page,
                                                 txn_mgr, txn, undo_mgr);
    ExecutorSeqScan scan(ctx, table_name);
    scan.Open();
    Tuple tuple;
    size_t count = 0;
    while (scan.Next(&tuple)) {
        ++count;
    }
    scan.Close();
    txn_mgr->Commit(txn->tx_id_);
    return count;
}

}  // namespace

TEST(MVCC, InsertScanDeleteScan) {
    // 用独立的数据文件，避免污染其它测试
    auto disk_manager = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manager);
    auto meta_page = std::make_shared<MetaPage>(buffer_manager);
    auto txn_mgr = std::make_shared<TransactionManager>();
    auto undo_mgr = std::make_shared<UndoLogManager>(buffer_manager);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "mvcc_user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);
    entry_id table_id = table->id_;

    std::string name = "name";
    table->AddColumn(ColumnEntry(table->GetNextColumnId(), name, ColumnTypes::VARCHAR));
    std::string age = "age";
    table->AddColumn(ColumnEntry(table->GetNextColumnId(), age, ColumnTypes::INTEGER));
    catalog->AddTable(std::move(table));

    // 在 meta 中登记该表（first/last 置 0，触发首次插入惰性分配）
    if (meta_page->items_.find(table_id) == meta_page->items_.end()) {
        MetaItem item;
        item.table_id_ = table_id;
        item.table_name_ = table_name;
        item.first_page_id = 0;
        item.last_page_id = 0;
        item.num_rows_ = 0;
        MetaColumnItem c1{name, ColumnTypes::VARCHAR, 0, 0};
        MetaColumnItem c2{age, ColumnTypes::INTEGER, 0, 1};
        item.items_.push_back(c1);
        item.items_.push_back(c2);
        meta_page->AddTable(item);
    }

    // 插入若干行（同一事务），足以触发翻页
    const int kRows = 200;
    {
        auto txn = txn_mgr->Begin();
        auto ctx = std::make_shared<ExecutorContext>(catalog, buffer_manager, meta_page,
                                                     txn_mgr, txn, undo_mgr);
        for (int i = 0; i < kRows; ++i) {
            Binder binder(catalog);
            Parser parser;
            parser.ParserSQL("insert into mvcc_user (name,age) values('u" + std::to_string(i) +
                             "'," + std::to_string(i) + ")");
            auto &stmt = parser.GetStatements()[0];
            auto use = std::unique_ptr<InsertStatement>(dynamic_cast<InsertStatement *>(stmt.release()));
            auto bound = binder.BoundInsertStatement(std::move(use));
            Planner planner;
            auto logical = planner.CreateLogicalPlan(std::move(bound));
            auto physical = planner.CreatePhysicalPlan(logical);
            ExecutorFactory factory(ctx);
            auto exec = factory.BuildExecutor(physical);
            Execute execute(ctx);
            execute.ExecuteInsert(std::move(exec));
        }
        txn_mgr->Commit(txn->tx_id_);
    }

    // 提交后另开事务扫描，应看到全部 kRows 行（验证翻页 + 可见性）
    size_t seen = CountVisibleRows(catalog, buffer_manager, meta_page, txn_mgr, undo_mgr, table_name);
    EXPECT_EQ(seen, static_cast<size_t>(kRows));

    // 删除全部可见行
    {
        auto txn = txn_mgr->Begin();
        auto ctx = std::make_shared<ExecutorContext>(catalog, buffer_manager, meta_page,
                                                     txn_mgr, txn, undo_mgr);
        ExecutorDelete del(ctx, table_name);
        del.Open();
        Tuple t;
        del.Next(&t);
        del.Close();
        EXPECT_EQ(del.GetDeletedCount(), static_cast<size_t>(kRows));
        txn_mgr->Commit(txn->tx_id_);
    }

    // 删除提交后扫描，应看到 0 行
    size_t after_delete = CountVisibleRows(catalog, buffer_manager, meta_page, txn_mgr, undo_mgr, table_name);
    EXPECT_EQ(after_delete, 0u);
}
