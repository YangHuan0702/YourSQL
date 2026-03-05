//
// Created by huan.yang on 2026-03-05.
//
#pragma once
#include "physical_expression.h"
#include "common/types/plan_operator_types.h"

namespace YourSQL {

    class PhysicalUnaryExpression : public PhysicalExpression {
    public:
        explicit PhysicalUnaryExpression(entry_id table_id,entry_id column_id) : PhysicalExpression() {}
        ~PhysicalUnaryExpression() override = default;

        auto Evaluate(const Tuple &tuple) const -> Value override {
            return Value(true);
        }

        entry_id table_id_;
        entry_id column_id_;
    };

}
