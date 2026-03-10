//
// Created by huan.yang on 2026-03-09.
//
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"
#include "parser/statement/insert_statement.h"

using namespace YourSQL;


TEST(Binder,BinderInsertSQLTest) {
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

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &insertStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class InsertStatement>(dynamic_cast<InsertStatement*>(insertStatement.release()));
        auto statement = binder.BoundInsertStatement(std::move(useStatement));
        std::cout << "Binder finish." << std::endl;
    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }


}

