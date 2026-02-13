//
// Created by huan.yang on 2026-02-12.
//

#pragma once
#include "parser/expression.h"
#include "parser/transformer.h"
#include "sql/Expr.h"

namespace YourSQL {
    class CompExpression : public BaseExpression {
    public:
        explicit CompExpression(OperatorType type, std::unique_ptr<BaseExpression> left,
                                std::unique_ptr<BaseExpression> right)
            : BaseExpression(ExpressionType::COMP), operator_type(type), left_(std::move(left)),right_(std::move(right)) {
        }

        ~CompExpression() override;

        auto to_string() -> std::string override;

        OperatorType operator_type;

        std::unique_ptr<BaseExpression> left_;
        std::unique_ptr<BaseExpression> right_;
    };
}
