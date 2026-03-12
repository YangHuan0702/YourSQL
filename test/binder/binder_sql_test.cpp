//
// Created by huan.yang on 2026-03-02.
//
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"
#include "executor/execute.h"
#include "executor/executor_factory.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;

TEST(Binder,BinderSQLTest) {
    std::string sql = "select name,age,email from user where name != 'yanghuan' and age < 20 or del = '0' group by age limit 0 offset 10";
    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(1,table_name);

    std::string name = "name";
    auto column_name = ColumnEntry(IdManager::GetNextEntryId(),name,ColumnTypes::VARCHAR);
    table->AddColumn(column_name);

    std::string age = "age";
    auto column_age = ColumnEntry(IdManager::GetNextEntryId(),age,ColumnTypes::INTEGER);
    table->AddColumn(column_age);

    std::string email = "email";
    auto column_email = ColumnEntry(IdManager::GetNextEntryId(),email,ColumnTypes::VARCHAR);
    table->AddColumn(column_email);

    std::string del = "del";
    auto column_del = ColumnEntry(IdManager::GetNextEntryId(),del,ColumnTypes::INTEGER);
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);
    auto meta_page = std::make_shared<MetaPage>(buffer_manager);
    try {
        if (disk_manger->Size() == 0) {
            meta_page->Init();
        } else {
            meta_page->ReadMata();
        }

        MetaItem meta_item;
        meta_item.table_id_ = 1;
        meta_item.table_name_ = table_name;
        meta_item.last_page_id = 0;
        meta_item.first_page_id = 0;
        meta_item.num_rows_ = 0;
        meta_page->AddTable(meta_item);
    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    auto executor_context = std::make_shared<ExecutorContext>(catalog,buffer_manager,meta_page);

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &selectStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class SelectStatement>(dynamic_cast<SelectStatement*>(selectStatement.release()));
        auto statement = binder.BoundSelectStatement(std::move(useStatement));

        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(statement));

        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        Execute execute(executor_context);

        ExecutorFactory factory(executor_context,1);

        auto executor = factory.BuildExecutor(physical_operator);
        execute.ExecuteQuery(std::move(executor));

    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
