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
                                          std::unique_ptr<PhysicalExpression> right)
            : op_(op), left_(std::move(left)), right_(std::move(right)) {
        }

        ~PhysicalBinaryExpression() override = default;

        auto Evaluate(const Tuple &tuple) const -> Value override {
            auto left_val = left_->Evaluate(tuple);
            auto right_val = right_->Evaluate(tuple);

            if (op_ == BinaryOp::AND || op_ == BinaryOp::OR) {
                return EvaluateLogical(left_val, right_val);
            }

            if (left_val.IsNull() || right_val.IsNull()) {
                return Value(); // NULL
            }

            if (op_ == BinaryOp::CANCAT || op_ == BinaryOp::LIKE || op_ == BinaryOp::NLIKE || op_ == BinaryOp::IN) {
                return EvaluateStringOp(left_val, right_val);
            }

            return EvaluateArithmeticOrComparison(left_val, right_val);
        }

    private:
        auto EvaluateLogical(const Value &left_val, const Value &right_val) const -> Value {
            bool left_null = left_val.IsNull();
            bool right_null = right_val.IsNull();

            if (op_ == BinaryOp::AND) {
                if (!left_null && !left_val.GetBool()) return Value(false);
                if (!right_null && !right_val.GetBool()) return Value(false);
                if (left_null || right_null) return Value();
                return Value(true);
            } else {
                if (!left_null && left_val.GetBool()) return Value(true);
                if (!right_null && right_val.GetBool()) return Value(true);
                if (left_null || right_null) return Value();
                return Value(false);
            }
        }

        auto EvaluateStringOp(const Value &left_val, const Value &right_val) const -> Value {
            auto l_str = left_val.GetString();
            auto r_str = right_val.GetString();

            if (op_ == BinaryOp::CANCAT) {
                return Value(l_str + r_str);
            }

            if (op_ == BinaryOp::LIKE || op_ == BinaryOp::NLIKE) {
                bool matches = (l_str.find(r_str) != std::string::npos);
                return Value(op_ == BinaryOp::LIKE ? matches : !matches);
            }

            // if (op_ == BinaryOp::IN) {
            //     throw std::runtime_error("IN operator logic is incomplete, requires a set/list.");
            // }
            throw std::runtime_error("Unsupported String Operation");
        }

        auto EvaluateArithmeticOrComparison(const Value &left_val, const Value &right_val) const -> Value {
            if (left_val.IsInt() && right_val.IsInt()) {
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
                    default: throw std::runtime_error("Unknown BinaryOp for Int");
                }
            }

            if ((left_val.IsInt() || left_val.IsDouble()) && (right_val.IsInt() || right_val.IsDouble())) {
                double l_double = left_val.IsInt() ? left_val.GetInt() : left_val.GetDouble();
                double r_double = right_val.IsInt() ? right_val.GetInt() : right_val.GetDouble();

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
                    default: throw std::runtime_error("Unknown BinaryOp for Double");
                }
            }

            if (left_val.IsString() && right_val.IsString()) {
                auto l_str = left_val.GetString();
                auto r_str = right_val.GetString();
                switch (op_) {
                    case BinaryOp::GT: return Value(l_str > r_str);
                    case BinaryOp::GTE: return Value(l_str >= r_str);
                    case BinaryOp::LT: return Value(l_str < r_str);
                    case BinaryOp::LTE: return Value(l_str <= r_str);
                    case BinaryOp::EQ: return Value(l_str == r_str);
                    case BinaryOp::NEQ: return Value(l_str != r_str);
                    default: throw std::runtime_error("Unknown BinaryOp for String");
                }
            }
            throw std::runtime_error("Type mismatch or unsupported operation between types");
        }

        BinaryOp op_;
        std::unique_ptr<PhysicalExpression> left_;
        std::unique_ptr<PhysicalExpression> right_;
    };
}
