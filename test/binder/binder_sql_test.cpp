//
// Created by huan.yang on 2026-03-02.
//
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"

using namespace YourSQL;

TEST(Binder,BinderSQLTest) {
    std::string sql = "select name,age,email from user where name != 'yanghuan' and age < 20 or del = '0' group by age limit 0 offset 10";
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

    table->AddColumn(column_name);
    table->AddColumn(column_age);
    table->AddColumn(column_email);
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    try {
        Binder binder(catalog);
        std::unique_ptr<BaseStatement> &selectStatement = parser.GetStatements()[0];
        auto useStatement = std::unique_ptr<class SelectStatement>(dynamic_cast<SelectStatement*>(selectStatement.release()));
        auto statement = binder.BoundSelectStatement(std::move(useStatement));
        std::cout << "Binder finish." << std::endl;
    }catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}
