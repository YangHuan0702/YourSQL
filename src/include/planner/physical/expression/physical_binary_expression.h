//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include <memory>

#include "physical_expression.h"
#include "common/types/plan_operator_types.h"

namespace YourSQL {


    class PhysicalBinaryExpression : public PhysicalExpression {
    public:
        explicit PhysicalBinaryExpression(BinaryOp op, std::unique_ptr<PhysicalExpression> left,
                                          std::unique_ptr<PhysicalExpression> right) : op_(op), left_(std::move(left)),
            right_(std::move(right)) {
        }

        ~PhysicalBinaryExpression() override = default;

        auto Evaluate(const Tuple &tuple) const -> Value override {
            auto l = left_->Evaluate(tuple).GetInt();
            auto r = right_->Evaluate(tuple).GetInt();
            switch (op_) {
                case BinaryOp::ADD: return Value(l + r);
                case BinaryOp::SUB: return Value(l - r);
                case BinaryOp::GT: return Value(l > r);
                case BinaryOp::GTE: return Value(l >= r);
                case BinaryOp::LT: return Value(l < r);
                case BinaryOp::LTE: return Value(l <= r);
                case BinaryOp::EQ: return Value(l == r);
                default: throw std::runtime_error("Unknown BinaryOp");
            }
        }
        BinaryOp op_;
        std::unique_ptr<PhysicalExpression> left_;
        std::unique_ptr<PhysicalExpression> right_;
    };
}
