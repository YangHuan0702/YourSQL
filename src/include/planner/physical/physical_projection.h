//
// Created by huan.yang on 2026-03-05.
//
#pragma once
#include "physical_operator.h"
#include "expression/physical_column_expression.h"

namespace YourSQL {
    class PhysicalProjection : public PhysicalOperator {
    public:
        explicit PhysicalProjection() : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_PROJECTION) {
        }

        ~PhysicalProjection() override = default;

        auto to_string() -> std::string override {
            return "";
        }

        std::vector<std::unique_ptr<PhysicalExpression> > columns_;
    };
}
