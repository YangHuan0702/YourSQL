//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"
#include "common/types/expression_types.h"

namespace YourSQL {

    class BoundCompExpression : public BoundExpression {
    public:
        explicit BoundCompExpression(OperatorType opType,ColumnTypes returnType) : BoundExpression(returnType), op_type_(opType){}
        ~BoundCompExpression() override = default;

        auto to_string() -> std::string override {
            return "BoundCompExpression";
        }

        OperatorType op_type_;
    };

}
