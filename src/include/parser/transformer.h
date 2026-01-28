//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <vector>

#include "SQLParserResult.h"
#include "expression/select_expression.h"
#include "sql/CreateStatement.h"
#include "parser/statement.h"
#include "statement/select_statement.h"

namespace YourSQL {
    class Transformer {
    public:
        Transformer() = default;

        auto transformStatement(hsql::SQLParserResult &sqlParserResult,
                                std::vector<std::unique_ptr<BaseStatement> > &targetStatements) -> bool;
    private:
        auto transformSelect(hsql::SelectStatement *sqlStatement) -> std::unique_ptr<SelectStatement>;

        auto transformStatement(const hsql::SQLStatement *sql_statement) -> std::unique_ptr<BaseStatement>;
    };
}
