//
// Created by 杨欢 on 2026/2/13.
//
#pragma once
#include <string>

#include "parser/expression.h"

namespace YourSQL {

    class LikeExpression : public BaseExpression {

    public:
        explicit LikeExpression(const std::string &column, const std::string &value) : BaseExpression(ExpressionType::EXPR),target_column(std::move(column)), like_value(std::move(value)) {}
        ~LikeExpression() override = default;

        auto to_string() -> std::string override {
            return "LikeExpression:[" + target_column+"-"+like_value+"]";
        }

        std::string target_column;
        std::string like_value;
    };

}
