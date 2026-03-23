//
// Created by 杨欢 on 2026/3/22.
//

#include "binder/binder.h"
#include "gtest/gtest.h"
#include "parser/parser.h"


using namespace YourSQL;

TEST(Binder, DeleteSQLTest) {
    std::string sql = "delete from user where name = '你好'";

    Parser parser;
    parser.ParserSQL(sql);
}