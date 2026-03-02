//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "logical_operator.h"
#include "binder/bound_expression.h"

namespace YourSQL {
    class LogicalProjection final : public LogicalOperator {
    public:
        explicit LogicalProjection(std::vector<std::unique_ptr<BoundExpression>> expressions) : LogicalOperator(LogicalOperatorType::LOGICAL_PROJECTION), expressions_(std::move(expressions)) {}
        ~LogicalProjection() override = default;

        auto to_string() -> std::string override {
            return "Logical Projection";
        }
        std::vector<std::unique_ptr<BoundExpression>> expressions_;
    };
}
