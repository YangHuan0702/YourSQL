//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/expression.h"

namespace YourSQL {
    class SelectExpression : public BaseExpression {
    public:
        SelectExpression() : BaseExpression(ExpressionType::SELECT) {}


    };
}
