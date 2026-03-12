//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "logical_operator.h"
#include "common/types/column_types.h"

namespace YourSQL {
    class LogicalValues : public LogicalOperator {
    public:
        explicit LogicalValues(std::vector<entry_id> column_ids, std::vector<Value> values) : LogicalOperator(
                LogicalOperatorType::LOGICAL_VALUES), column_ids_(std::move(column_ids)),values_(std::move(values)) {
        }

        ~LogicalValues() override = default;

        auto to_string() -> std::string override {
            return "LogicalValues";
        }

        std::vector<entry_id> column_ids_;
        std::vector<Value> values_;
    };
}
