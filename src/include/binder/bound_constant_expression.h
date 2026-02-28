//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"
#include "catalog/column_entry.h"
#include "parser/expression/constant_expression.h"

namespace YourSQL {

    class BoundConstantExpression : public BoundExpression{
    public:
        explicit BoundConstantExpression(std::unique_ptr<ConstantExpression> constExpression);
        ~BoundConstantExpression() override = default;

        auto to_string() -> std::string override;

        ColumnEntry column_entry;
        Value target_value;
    };

}
