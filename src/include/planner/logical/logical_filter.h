//
// Created by huan.yang on 2026-03-02.
//

#pragma once
#include "logical_operator.h"
#include "binder/bound_expression.h"

namespace YourSQL {

    class LogicalFilter final : public LogicalOperator {
    public:
        explicit LogicalFilter(std::vector<std::unique_ptr<BoundExpression>> expressions) : LogicalOperator(LogicalOperatorType::LOGICAL_FILTER),expressions_(std::move(expressions)) {}
        ~LogicalFilter() override = default;

        auto to_string() -> std::string override {
            return "LogicalFilter";
        }

        std::vector<std::unique_ptr<BoundExpression>> expressions_;
    };

}
