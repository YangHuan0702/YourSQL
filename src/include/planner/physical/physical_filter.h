//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "physical_operator.h"
#include "expression/physical_expression.h"

namespace YourSQL {

    class PhysicalFilter : public PhysicalOperator {
    public:
        explicit PhysicalFilter() : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_FILTER){}
        ~PhysicalFilter() override = default;

        auto to_string() -> std::string override {
            return "";
        }

        std::unique_ptr<PhysicalExpression> expressions_{};

    };

}
