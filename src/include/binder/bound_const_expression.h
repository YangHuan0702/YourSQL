//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "bound_expression.h"

namespace YourSQL {

    class BoundConstExpression : public BoundExpression {
    public:
        explicit BoundConstExpression(Value value) : BoundExpression(ColumnTypes::INVALID), value_(std::move(value)) {}
        ~BoundConstExpression() override = default;

        auto to_string() -> std::string override {
            return std::to_string(value_.GetBool());
        }

        Value value_;
    };

}
