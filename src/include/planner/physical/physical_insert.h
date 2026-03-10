//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "physical_operator.h"
#include "common/type.h"

namespace YourSQL {
    class PhysicalInsert : public PhysicalOperator {
    public:
        explicit PhysicalInsert(entry_id table_id, std::vector<entry_id> column_ids) : PhysicalOperator(
                PhysicalOperatorTypes::PHYSICAL_INSERT), table_id_(table_id), column_ids_(std::move(column_ids)) {
        }

        ~PhysicalInsert() override = default;


        auto to_string() -> std::string override {
            return "PhysicalInsert";
        }

        entry_id table_id_;
        std::vector<entry_id> column_ids_;
    };
}
