//
// Created by huan.yang on 2026-02-12.
//

#pragma once
#include "parser/expression.h"
#include "parser/transformer.h"

namespace YourSQL {
    class BinaryExpression : public BaseExpression {
    public:
        explicit BinaryExpression(std::unique_ptr<BaseExpression> left,
                                std::unique_ptr<BaseExpression> right,BinaryOp op)
            : BaseExpression(ExpressionType::BINARY), left_(std::move(left)),right_(std::move(right)),op_(op) {
        }
        BinaryExpression(const BinaryExpression &copy) noexcept = delete;
        ~BinaryExpression() override = default;

        auto to_string() -> std::string override {
            return "CompExpression";
        }

        std::unique_ptr<BaseExpression> left_;
        std::unique_ptr<BaseExpression> right_;
        BinaryOp op_;
    };
}
