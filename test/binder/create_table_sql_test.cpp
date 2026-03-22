//
// Created by 杨欢 on 2026/3/22.
//


#include "gtest/gtest.h"
#include "parser/parser.h"
using namespace YourSQL;

TEST(Binder,CreateTableSQLTest) {
    std::string sql = "CREATE TABLE user (age INT,name VARCHAR(50),email VARCHAR(50),del INT);";
    Parser parser;
    parser.ParserSQL(sql);
}
