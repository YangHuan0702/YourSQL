//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include <utility>

#include "executor.h"

namespace YourSQL {
    class ExecutorCreateTable : public Executor {
    public:
        explicit ExecutorCreateTable(std::shared_ptr<ExecutorContext> context, entry_id table_id) : Executor(
            std::move(context), PhysicalOperatorTypes::PHYSICAL_CREATE_TABLE),table_id_(table_id) {
        }

        ~ExecutorCreateTable() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        entry_id table_id_;
    };
}
