//
// Created by huan.yang on 2026-01-28.
//
#include "gtest/gtest.h"
#include "parser/parser.h"

TEST(Parser, ParserSelectSQLTest) {
    std::string sql = "select name,age,email from user where name != 'yanghuan' and age < 20 or del = 0 group by age limit 0 offset 10";
    YourSQL::Parser parser;
    parser.ParserSQL(sql);
}