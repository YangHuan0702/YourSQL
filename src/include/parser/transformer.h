//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <vector>

#include "SQLParserResult.h"
#include "common/types/column_types.h"
#include "common/types/plan_operator_types.h"
#include "expression/select_expression.h"
#include "sql/CreateStatement.h"
#include "parser/statement.h"
#include "sql/InsertStatement.h"
#include "sql/Table.h"
#include "statement/insert_statement.h"
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
        auto transformInsert(hsql::InsertStatement*) -> std::unique_ptr<InsertStatement>;

        auto transformWhere(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformTableRef(hsql::TableRef *table_ref) -> std::unique_ptr<YourTable>;

        auto transformOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformStatement(const hsql::SQLStatement *sql_statement) -> std::unique_ptr<BaseStatement>;

        auto transformColumnExpr(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformConstExpr(ColumnTypes type,Value value) -> std::unique_ptr<BaseExpression>;

        auto transformLikeExpr(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformInExpr(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformIsNullOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformLogicExpr(hsql::Expr *expr, const std::string &table_name) -> std::unique_ptr<BaseExpression>;

        auto transformBinaryOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        auto transformUnaryOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression>;

        static auto transformOperatorType(hsql::OperatorType type) -> BinaryOp;
    };
}
