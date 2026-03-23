//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include <utility>

#include "executor.h"
#include "planner/physical/physical_operator.h"

namespace YourSQL {

    class ExecutorFactory {
    public:
        explicit ExecutorFactory(std::shared_ptr<ExecutorContext> context,entry_id table_id) : context_(std::move(context)),table_id_(table_id) {}

        ~ExecutorFactory() = default;

        auto BuildExecutor(std::unique_ptr<PhysicalOperator> &physical_operator) -> std::unique_ptr<Executor>;

        std::shared_ptr<ExecutorContext> context_;
        entry_id table_id_;
    };

}
