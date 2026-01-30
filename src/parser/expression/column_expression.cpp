//
// Created by huan.yang on 2026-01-30.
//
#include "parser/expression/column_expression.h"
using namespace YourSQL;


auto ColumnExpression::to_string() -> std::string {
    return column_name + "," + alias;
}

