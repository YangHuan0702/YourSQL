//
// Created by huan.yang on 2026-01-30.
//
#include "parser/expression/column_expression.h"
#include "parser/transformer.h"
using namespace YourSQL;


auto Transformer::transformColumnExpr(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    std::string name = expr->name;
    std::string alias = "";
    if (expr->alias != nullptr) {
        alias = expr->alias;
    }
    return std::make_unique<ColumnExpression>(name,alias);
}



auto ColumnExpression::to_string() -> std::string {
    return column_name + "," + alias;
}

