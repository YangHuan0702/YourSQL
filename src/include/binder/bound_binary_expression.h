//
// Created by huan.yang on 2026-03-05.
//
#pragma once
#include "bound_expression.h"
#include "common/type.h"
#include "common/types/plan_operator_types.h"

namespace YourSQL {
    class BoundBinaryExpression : public BoundExpression {
    public:
        explicit BoundBinaryExpression(BinaryOp opType) : BoundExpression(ColumnTypes::INTEGER,
                ExpressionType::BINARY),
            binary_op_(opType) {
        }

        ~BoundBinaryExpression() override = default;

        auto to_string() -> std::string override {
            return "BoundBinaryExpression[]";
        }

        BinaryOp binary_op_;
    };
}
