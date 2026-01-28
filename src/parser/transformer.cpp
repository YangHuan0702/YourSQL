//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"

#include <algorithm>

#include "sql/SelectStatement.h"

using namespace YourSQL;

auto Transformer::transformStatement(hsql::SQLParserResult &sqlParserResult, std::vector<std::unique_ptr<BaseStatement> > &targetStatements) -> bool {
    for (size_t index = 0L; index < sqlParserResult.size(); ++index) {
        auto stmt = sqlParserResult.getStatement(index);
        auto res = transformStatement(stmt);
        if (!res) {
            targetStatements.clear();
            return false;
        }
        targetStatements.push_back(std::move(res));
    }
    return true;
}

auto Transformer::transformStatement(const hsql::SQLStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {return nullptr;}
    switch (sql_statement->type()) {
        case hsql::kStmtSelect:
            return transformSelect((hsql::SelectStatement*)sql_statement);
        default:
            throw std::runtime_error("Invalid SQL statement");
    }
}


