//
// Created by huan.yang on 2026-01-28.
//
#include "parser/parser.h"

#include "SQLParser.h"
#include "SQLParserResult.h"
#include "glog/logging.h"
#include "parser/transformer.h"

using namespace YourSQL;

auto Parser::ParserSQL(const std::string &sql) -> void {
    LOG(INFO) << "[Parser] Parsing SQL:" << sql;

    hsql::SQLParserResult result;
    hsql::SQLParser::parse(sql,&result);

    if (!result.isValid()) {
        LOG(ERROR) << "[Parser] SQL: " << sql << "parsed invalid : " <<result.errorMsg() << ", " << result.errorLine() << "," << result.errorColumn();
        return;
    }

    Transformer transfomer;
    transfomer.transformStatement(result,statements);
}


