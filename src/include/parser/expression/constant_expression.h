//
// Created by huan.yang on 2026-02-12.
//
#pragma once
#include "parser/expression.h"
#include "common/types/column_types.h"

namespace YourSQL {

    class ConstantExpression: public BaseExpression {
    public:
        explicit ConstantExpression(ColumnTypes type, Value value) : BaseExpression(ExpressionType::CONST) {}
    };


}
