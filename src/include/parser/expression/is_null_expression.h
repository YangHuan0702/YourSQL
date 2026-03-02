//
// Created by 杨欢 on 2026/2/13.
//
#pragma once
#include <string>

#include "parser/expression.h"

namespace YourSQL {
    class IsNullExpression : public BaseExpression {
    public:
        explicit IsNullExpression(const std::string &column) :BaseExpression(ExpressionType::OPERATOR),target_column(std::move(column)){}
        ~IsNullExpression() override = default;

        auto to_string() -> std::string override {
            return "IsNullExpression:[" + target_column + "]";
        }

        std::string target_column;
    };
}
