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
            std::unique_ptr<PhysicalProjection> projection) :Executor(context,PhysicalOperatorTypes::PHYSICAL_PROJECTION), projection_(std::move(projection)) {}
        ~ExecutorProjection() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next() -> bool override;

        std::unique_ptr<PhysicalProjection> projection_;
    };

}
