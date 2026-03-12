//
// Created by huan.yang on 2026-03-10.
//

#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"
#include "executor/execute.h"
#include "executor/executor_factory.h"
#include "glog/logging.h"
#include "parser/statement/insert_statement.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;


TEST(Executor, ExecutorInsertSQLTest) {
    std::string sql = "insert into user (name,age) values('你好',23)";

    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);
    auto table_id = table->id_;

    std::string name = "name";
    auto column_name = ColumnEntry(table->GetNextColumnId(), name, ColumnTypes::VARCHAR);
    table->AddColumn(column_name);

    std::string age = "age";
    auto column_age = ColumnEntry(table->GetNextColumnId(), age, ColumnTypes::INTEGER);
    table->AddColumn(column_age);

    std::string email = "email";
    auto column_email = ColumnEntry(table->GetNextColumnId(), email, ColumnTypes::VARCHAR);
    table->AddColumn(column_email);

    std::string del = "del";
    auto column_del = ColumnEntry(table->GetNextColumnId(), del, ColumnTypes::INTEGER);
    column_del.default_value_ = Value(0);
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_shared<MetaPage>(buffer_manager);
    try {
        if (disk_manger->Size() == 0) {
            meta_page->Init();

            MetaItem meta_item;
            meta_item.table_id_ = table_id;
            meta_item.table_name_ = table_name;
            meta_item.last_page_id = 0;
            meta_item.first_page_id = 0;
            meta_item.num_rows_ = 0;
            meta_item.offset = sizeof(size_t) * 3;
            meta_page->AddTable(meta_item);
        } else {
            meta_page->ReadMata();
        }
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    auto executor_context = std::make_shared<ExecutorContext>(catalog, buffer_manager, meta_page);

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &insertStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class InsertStatement>(
            dynamic_cast<InsertStatement *>(insertStatement.release()));
        auto statement = binder.BoundInsertStatement(std::move(useStatement));

        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(statement));

        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        Execute execute(executor_context);

        ExecutorFactory factory(executor_context, table_id);

        auto executor = factory.BuildExecutor(physical_operator);
        execute.ExecuteInsert(std::move(executor));
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
