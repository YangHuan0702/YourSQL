//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include "physical_expression.h"

namespace YourSQL {

    class PhysicalConstantExpression : public PhysicalExpression {
    public:
        explicit PhysicalConstantExpression(Value value) : value_(value){}
        ~PhysicalConstantExpression() override = default;

        Value Evaluate(const Tuple &tuple) const override {
            return value_;
        }

        Value value_;
    };

}
