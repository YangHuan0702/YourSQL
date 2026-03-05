//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"
#include "common/type.h"
#include "common/types/expression_types.h"
#include "common/types/plan_operator_types.h"

namespace YourSQL {
    class BoundUnaryExpression : public BoundExpression {
    public:
        explicit BoundUnaryExpression(UnaryOp opType, entry_id table_id_, entry_id column_id) : BoundExpression(
                ColumnTypes::INTEGER, ExpressionType::UNARY),
            table_id_(table_id_), column_id_(column_id), op_type_(opType) {
        }

        ~BoundUnaryExpression() override = default;

        auto to_string() -> std::string override {
            return "BoundUnaryExpression";
        }

        entry_id table_id_;
        entry_id column_id_;
        UnaryOp op_type_;
    };
}
