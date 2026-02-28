//
// Created by 杨欢 on 2026/2/28.
//
#pragma once
#include "binder/bound_expression.h"
#include "catalog/column_entry.h"

namespace YourSQL {

    class BoundStarExpression {
    public:
        explicit BoundStarExpression() {
        }
        ~BoundStarExpression() = default;

        auto to_string() -> std::string {
            return "BoundStarExpression";
        }

        std::vector<std::unique_ptr<BoundColumnRefExpression>> columns_;
    };

}
