//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "executor.h"
#include "planner/physical/expression/physical_expression.h"

namespace YourSQL {

    class ExecutorFilter : public Executor {
    public:
        explicit ExecutorFilter(std::shared_ptr<ExecutorContext> context, std::unique_ptr<PhysicalExpression> expression)
        : Executor(context,PhysicalOperatorTypes::PHYSICAL_FILTER),expression_(std::move(expression)) {}
        ~ExecutorFilter() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        std::unique_ptr<PhysicalExpression> expression_;
    };

}
