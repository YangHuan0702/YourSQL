//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "logical_operator.h"
#include "common/types/column_types.h"
#include "common/type.h"

namespace YourSQL {
    class LogicalValues : public LogicalOperator {
    public:
        explicit LogicalValues(std::vector<entry_id> column_ids, std::vector<Value> values,entry_id table_id) : LogicalOperator(
                LogicalOperatorType::LOGICAL_VALUES), column_ids_(std::move(column_ids)),values_(std::move(values)),table_id_(table_id) {
        }

        ~LogicalValues() override = default;

        auto to_string() -> std::string override {
            return "LogicalValues";
        }

        std::vector<entry_id> column_ids_;
        std::vector<Value> values_;
        entry_id table_id_;
    };
}
