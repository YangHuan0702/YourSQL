//
// Created by huan.yang on 2026-01-30.
//
#include <iostream>

#include "parser/transformer.h"
using namespace YourSQL;

auto Transformer::transformWhere(hsql::Expr *expr,std::unique_ptr<YourTable> &table) -> std::unique_ptr<BaseExpression> {
    if (!expr) return nullptr;
    switch (expr->type) {
        case hsql::kExprOperator:
            return transformOperator(expr,table->table_name_);
        default:
            throw std::runtime_error("Invalid Where SQL expression");
    }
    std::string exprName = std::string(expr->name);
    std::cout << exprName << std::endl;
    return nullptr;
}
