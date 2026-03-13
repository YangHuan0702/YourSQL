//
// Created by huan.yang on 2026-03-12.
//
#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;


TEST(Executor, ExecutorSelectTest) {
    std::string sql =
            "select name,age,email from user where name != 'yanghuan' and age > 20 or del = 0 group by age limit 0 offset 10";
    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);
    entry_id table_id = table->id_;

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
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_shared<MetaPage>(buffer_manager);
    meta_page->ReadMata();

    auto executor_context = std::make_shared<ExecutorContext>(catalog, buffer_manager, meta_page);

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &selectStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class SelectStatement>(
            dynamic_cast<SelectStatement *>(selectStatement.release()));
        auto statement = binder.BoundSelectStatement(std::move(useStatement));

        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(statement));

        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        Execute execute(executor_context);

        ExecutorFactory factory(executor_context, table_id);

        auto executor = factory.BuildExecutor(physical_operator);

        execute.ExecuteQuery(std::move(executor));
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
