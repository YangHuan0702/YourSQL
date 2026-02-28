//
// Created by 杨欢 on 2026/2/14.
//

#pragma once
#include "bound_expression.h"
#include "catalog/column_entry.h"

namespace YourSQL {

    class BoundIsNullExpression : public BoundExpression {
    public:
        explicit BoundIsNullExpression(std::string &column_name);
        ~BoundIsNullExpression() override = default;

        auto to_string() -> std::string override;

        ColumnEntry column_entry;
    };
}
