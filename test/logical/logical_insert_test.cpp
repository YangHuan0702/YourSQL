//
// Created by huan.yang on 2026-03-10.
//
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"
#include "executor/execute.h"
#include "executor/executor_insert.h"
#include "parser/statement/insert_statement.h"
#include "planner/planner.h"
#include "planner/physical/physical_insert.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;


TEST(Logical,LogicalInsertSQLTest) {
    std::string sql = "insert into user (name,age) values('你好',23)";

    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(),table_name);

    std::string name = "name";
    auto column_name = ColumnEntry(IdManager::GetNextEntryId(),name,ColumnTypes::VARCHAR);

    std::string age = "age";
    auto column_age = ColumnEntry(IdManager::GetNextEntryId(),age,ColumnTypes::INTEGER);

    std::string email = "email";
    auto column_email = ColumnEntry(IdManager::GetNextEntryId(),email,ColumnTypes::VARCHAR);

    std::string del = "del";
    auto column_del = ColumnEntry(IdManager::GetNextEntryId(),del,ColumnTypes::INTEGER);
    column_del.default_value_ = Value(0);

    table->AddColumn(column_name);
    table->AddColumn(column_age);
    table->AddColumn(column_email);
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    auto disk_manger = std::make_unique<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(std::move(disk_manger));

    auto meta_page = std::make_shared<MetaPage>();
    meta_page->Init(buffer_manager);

    auto executor_context = std::make_shared<ExecutorContext>(catalog,buffer_manager,meta_page);

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &insertStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class InsertStatement>(dynamic_cast<InsertStatement*>(insertStatement.release()));
        auto statement = binder.BoundInsertStatement(std::move(useStatement));

        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(statement));

        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        Execute execute(executor_context);

        auto ph = dynamic_cast<PhysicalInsert*>(physical_operator.release());

        auto executor_insert = std::make_unique<ExecutorInsert>(executor_context,ph->table_id_,ph->column_ids_);

        execute.ExecuteInsert(std::move(executor_insert));
    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}