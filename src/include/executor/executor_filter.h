//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "executor.h"
#include "planner/physical/expression/physical_expression.h"

namespace YourSQL {

    class ExecutorFilter : public Executor {
    public:
        explicit ExecutorFilter(std::shared_ptr<ExecutorContext> context,std::vector<std::unique_ptr<PhysicalExpression>> &expressions)
        : Executor(context,PhysicalOperatorTypes::PHYSICAL_FILTER),expressions_(std::move(expressions)) {}
        ~ExecutorFilter() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next() -> bool override;

        std::vector<std::unique_ptr<PhysicalExpression>> expressions_;
    };

}
