//
// Created by huan.yang on 2026-01-28.
//
#include <iostream>
#include "SQLParser.h"

#include "gtest/gtest.h"
#include "util/sqlhelper.h"

TEST(PARSER, sqlParserTest) {
    std::string sql = "select * from user where name != 'yanghuan' limit0,10 group by age desc";
    hsql::SQLParserResult result;
    hsql::SQLParser::parse(sql,&result);
    for (auto i = 0u; i < result.size(); ++i) {
        // Print a statement summary.
        hsql::printStatementInfo(result.getStatement(i));
    }
    std::cout << "Success: " <<  (result.isValid() ? "Y" : "N")  << std::endl;
}