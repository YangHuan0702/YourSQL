//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "logical_operator.h"
#include "common/types/column_types.h"

namespace YourSQL {
    class LogicalValues : public LogicalOperator {
    public:
        explicit LogicalValues(std::vector<Value> values) : LogicalOperator(
                                                                LogicalOperatorType::LOGICAL_VALUES),
                                                            values_(std::move(values)) {
        }

        ~LogicalValues() override = default;

        auto to_string() -> std::string override {
            return "LogicalValues";
        }
        std::vector<Value> values_;
    };
}
