//
// Created by 杨欢 on 2026/2/13.
//
#pragma once
#include <string>
#include <utility>
#include "common/types/column_types.h"
#include "parser/expression.h"

namespace YourSQL {

    class LogicExpression : public BaseExpression {
    public:
        explicit LogicExpression(const OperatorType optType,std::string column, Value value) : BaseExpression(ExpressionType::EXPR), type_(optType),target_column(std::move(column)),value(std::move(value)) {}
        ~LogicExpression() override = default;

        auto to_string() -> std::string override {
            return "LogicExpression";
        }

        OperatorType type_;
        std::string target_column;
        Value value;
    };

}
