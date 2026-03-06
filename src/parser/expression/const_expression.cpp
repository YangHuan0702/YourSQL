//
// Created by huan.yang on 2026-03-06.
//
#include <iostream>

#include "parser/transformer.h"
#include "parser/expression/constant_expression.h"

using namespace YourSQL;


auto Transformer::transformConstExpr(ColumnTypes column_types,Value value) -> std::unique_ptr<BaseExpression> {
    return std::make_unique<ConstantExpression>(column_types,value);
}

