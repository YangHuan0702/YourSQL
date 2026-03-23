//
// Created by huan.yang on 2026-03-23.
//

#pragma once
#include "logical_operator.h"
#include "common/type.h"

namespace YourSQL {
    class LogicalCreateTable : public LogicalOperator {
    public:
        explicit LogicalCreateTable(entry_id table_id) : LogicalOperator(LogicalOperatorType::LOGICAL_CREATE_TABLE), table_id_(table_id){}

        ~LogicalCreateTable() override = default;

        auto to_string() -> std::string override {
            return "LogicalCreateTable";
        }

        entry_id table_id_;
    };
}
