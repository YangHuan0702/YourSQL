//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "executor.h"
#include "planner/physical/physical_projection.h"

namespace YourSQL {
    class ExecutorProjection final : public Executor {
    public:
        explicit ExecutorProjection(std::shared_ptr<ExecutorContext> context,
                                    const std::vector<entry_id> &column_ids) : Executor(context,
                                                                            PhysicalOperatorTypes::PHYSICAL_PROJECTION),
                                                                        column_ids_(column_ids) {
        }

        ~ExecutorProjection() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        std::vector<entry_id> column_ids_;
    };
}
