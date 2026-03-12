//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include <utility>

#include "physical_operator.h"
#include "common/types/column_types.h"

namespace YourSQL {
    class PhysicalValues : public PhysicalOperator {
    public:
        explicit PhysicalValues(std::vector<entry_id> column_ids,std::vector<Value> values) : PhysicalOperator(
                PhysicalOperatorTypes::PHYSICAL_VALUES), column_ids_(std::move(column_ids)), values_(std::move(values)) {
        }

        ~PhysicalValues() override = default;


        auto to_string() -> std::string override {
            return "PhysicalValues";
        }

        std::vector<entry_id> column_ids_;
        std::vector<Value> values_;
    };
}
