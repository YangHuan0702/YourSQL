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
            auto left_val = left_->Evaluate(tuple);
            auto right_val = right_->Evaluate(tuple);

            if (left_val.IsNull() || right_val.IsNull()) {
                if (op_ == BinaryOp::AND) {
                    if (!left_val.IsNull() && !left_val.GetBool()) return Value(false);
                    if (!right_val.IsNull() && !right_val.GetBool()) return Value(false);
                    return Value(); // NULL
                }
                if (op_ == BinaryOp::OR) {
                    if (!left_val.IsNull() && left_val.GetBool()) return Value(true);
                    if (!right_val.IsNull() && right_val.GetBool()) return Value(true);
                    return Value(); // NULL
                }
                return Value(); // NULL
            }

            if (op_ == BinaryOp::AND) {
                return Value(left_val.GetBool() && right_val.GetBool());
            }
            if (op_ == BinaryOp::OR) {
                return Value(left_val.GetBool() || right_val.GetBool());
            }

            if (op_ == BinaryOp::LIKE || op_ == BinaryOp::NLIKE || op_ == BinaryOp::CANCAT) {
                auto l_str = left_val.GetString();
                auto r_str = right_val.GetString();
                if (op_ == BinaryOp::CANCAT) {
                    return Value(l_str + r_str);
                }
                bool matches = (l_str.find(r_str) != std::string::npos);
                return Value(op_ == BinaryOp::LIKE ? matches : !matches);
            }

            // IN 操作符
            if (op_ == BinaryOp::IN) {
                auto l_str = left_val.GetString();
                auto r_str = right_val.GetString();
                return Value(l_str == r_str);
            }

            try {
                int l_int = left_val.GetInt();
                int r_int = right_val.GetInt();

                switch (op_) {
                    case BinaryOp::ADD: return Value(l_int + r_int);
                    case BinaryOp::SUB: return Value(l_int - r_int);
                    case BinaryOp::MU: return Value(l_int * r_int);
                    case BinaryOp::DE:
                        if (r_int == 0) throw std::runtime_error("Division by zero");
                        return Value(l_int / r_int);
                    case BinaryOp::GT: return Value(l_int > r_int);
                    case BinaryOp::GTE: return Value(l_int >= r_int);
                    case BinaryOp::LT: return Value(l_int < r_int);
                    case BinaryOp::LTE: return Value(l_int <= r_int);
                    case BinaryOp::EQ: return Value(l_int == r_int);
                    case BinaryOp::NEQ: return Value(l_int != r_int);
                    default: break;
                }
            } catch (const std::bad_variant_access&) {

            }

            try {
                double l_double = left_val.GetDouble();
                double r_double = right_val.GetDouble();

                switch (op_) {
                    case BinaryOp::ADD: return Value(l_double + r_double);
                    case BinaryOp::SUB: return Value(l_double - r_double);
                    case BinaryOp::MU: return Value(l_double * r_double);
                    case BinaryOp::DE:
                        if (r_double == 0.0) throw std::runtime_error("Division by zero");
                        return Value(l_double / r_double);
                    case BinaryOp::GT: return Value(l_double > r_double);
                    case BinaryOp::GTE: return Value(l_double >= r_double);
                    case BinaryOp::LT: return Value(l_double < r_double);
                    case BinaryOp::LTE: return Value(l_double <= r_double);
                    case BinaryOp::EQ: return Value(l_double == r_double);
                    case BinaryOp::NEQ: return Value(l_double != r_double);
                    default: break;
                }
            } catch (const std::bad_variant_access&) {
            }

            // 字符串比较
            try {
                auto l_str = left_val.GetString();
                auto r_str = right_val.GetString();

                switch (op_) {
                    case BinaryOp::GT: return Value(l_str > r_str);
                    case BinaryOp::GTE: return Value(l_str >= r_str);
                    case BinaryOp::LT: return Value(l_str < r_str);
                    case BinaryOp::LTE: return Value(l_str <= r_str);
                    case BinaryOp::EQ: return Value(l_str == r_str);
                    case BinaryOp::NEQ: return Value(l_str != r_str);
                    default: break;
                }
            } catch (const std::bad_variant_access&) {
                // 字符串类型转换失败
            }

            throw std::runtime_error("Unknown BinaryOp or type mismatch");
        }
        BinaryOp op_;
        std::unique_ptr<PhysicalExpression> left_;
        std::unique_ptr<PhysicalExpression> right_;
    };
}
