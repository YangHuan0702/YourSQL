//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/expression.h"

namespace YourSQL {

    class StarExpression : public BaseExpression {
    public:
        explicit StarExpression() : BaseExpression(ExpressionType::STAR) {}
        auto to_string() -> std::string {
            return "*";
        }
    };

}
