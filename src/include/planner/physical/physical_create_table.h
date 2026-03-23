//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include "physical_operator.h"
#include "common/type.h"

namespace YourSQL {

    class PhysicalCreateTable : public PhysicalOperator {
    public:
        explicit PhysicalCreateTable(entry_id table_id): PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_CREATE_TABLE), table_id_(table_id) {}

        ~PhysicalCreateTable() override = default;

        auto to_string() -> std::string override {
            return "PhysicalCreateTable(" + std::to_string(table_id_) + ")";
        }

        entry_id table_id_;
    };

}
