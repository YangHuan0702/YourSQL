//
// Created by huan.yang on 2026-03-05.
//
#pragma once
#include "common/types/plan_operator_types.h"
#include "parser/expression.h"

namespace YourSQL {

    class UnaryExpression : public BaseExpression {
    public:
        explicit UnaryExpression(std::string &column_name,UnaryOp op) : BaseExpression(ExpressionType::UNARY), column_name_(column_name), op_(op) {}
        ~UnaryExpression() override = default;

        auto to_string() -> std::string override {
            return "UnaryExpression";
        }


        std::string column_name_;
        UnaryOp op_;
    };

}
