//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"

namespace YourSQL {

    class BoundCompExpression : public BoundExpression {
    public:
        explicit BoundCompExpression();
        ~BoundCompExpression() override = default;

        auto to_string() -> std::string override;
    };

}
