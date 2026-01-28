//
// Created by huan.yang on 2026-01-28.
//
#include "gtest/gtest.h"
#include "parser/parser.h"

TEST(Parser, ParserSelectSQLTest) {
    std::string sql = "select * from user where name != 'yanghuan' limit0,10 group by age desc";
    YourSQL::Parser parser;
    parser.ParserSQL(sql);
}