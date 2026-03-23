//
// Created by 杨欢 on 2026/3/22.
//


#include "binder/binder.h"
#include "gtest/gtest.h"
#include "parser/parser.h"
using namespace YourSQL;

TEST(Binder,CreateTableSQLTest) {
    std::string sql = "CREATE TABLE user (age INT,name VARCHAR(50),email VARCHAR(50),del INT);";
    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();

    Binder binder(catalog);
    std::unique_ptr<BaseStatement> &create_table_statement = parser.GetStatements()[0];

}
