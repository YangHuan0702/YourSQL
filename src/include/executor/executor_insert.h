//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "executor.h"

namespace YourSQL {
    class ExecutorInsert : public Executor {
    public:
        explicit ExecutorInsert(std::shared_ptr<ExecutorContext> context, entry_id tableId,
                                std::vector<entry_id> columnIds) : Executor(
            context, PhysicalOperatorTypes::PHYSICAL_INSERT),table_id_(tableId),column_ids_(columnIds) {
        }
        ~ExecutorInsert() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        entry_id table_id_;
        std::vector<entry_id> column_ids_;


        TablePage *page_{nullptr};

    };
}
