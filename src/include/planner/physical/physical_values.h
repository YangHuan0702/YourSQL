//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "physical_operator.h"
#include "common/type.h"
#include "common/types/column_types.h"
#include "planner/logical/logical_operator.h"

namespace YourSQL {
    class PhysicalValues : public PhysicalOperator {
    public:
        explicit PhysicalValues(std::vector<Value> values) : PhysicalOperator(
                PhysicalOperatorTypes::PHYSICAL_VALUES),  values_(values) {
        }

        ~PhysicalValues() override = default;


        auto to_string() -> std::string override {
            return "PhysicalValues";
        }

        std::vector<Value> values_;
    };
}
