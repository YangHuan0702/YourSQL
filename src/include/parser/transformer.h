//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <vector>

#include "SQLParserResult.h"
#include "expression/select_expression.h"
#include "sql/CreateStatement.h"
#include "parser/statement.h"
#include "sql/Table.h"
#include "statement/select_statement.h"
#include "table_ref/your_table.h"

namespace YourSQL {
    class Transformer {
    public:
        Transformer() = default;

        auto transformStatement(hsql::SQLParserResult &sqlParserResult,
                                std::vector<std::unique_ptr<BaseStatement> > &targetStatements) -> bool;
    private:
        auto transformSelect(hsql::SelectStatement *sqlStatement) -> std::unique_ptr<SelectStatement>;
        auto transformWhere(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;
        auto transformTableRef(hsql::TableRef *table_ref) -> std::unique_ptr<YourTable>;

        auto transformOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;
        auto transformStatement(const hsql::SQLStatement *sql_statement) -> std::unique_ptr<BaseStatement>;
    };
}
