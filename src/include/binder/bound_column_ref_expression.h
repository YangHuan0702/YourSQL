//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"
#include "catalog/catalog.h"

namespace YourSQL {

    class BoundColumnRefExpression : public BoundExpression{
    public:
        explicit BoundColumnRefExpression(std::string &name);
        ~BoundColumnRefExpression() override = default;

        auto to_string() -> std::string override;

        ColumnEntry column_entry;
    };

}
