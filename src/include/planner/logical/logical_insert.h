//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "logical_operator.h"
#include "common/type.h"

namespace YourSQL {
    class LogicalInsert : public LogicalOperator {
    public:
        explicit LogicalInsert(entry_id table_id, std::vector<entry_id> column_ids) : LogicalOperator(
                LogicalOperatorType::LOGICAL_INSERT), table_id_(table_id), column_ids_(std::move(column_ids)) {
        }

        ~LogicalInsert() override = default;

        auto to_string() -> std::string override {
            return "LogicalInsert";
        }

        entry_id table_id_;
        std::vector<entry_id> column_ids_;
    };
}
