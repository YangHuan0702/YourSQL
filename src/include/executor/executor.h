//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include <memory>

#include "executor_context.h"
#include "common/types/physical_types.h"
#include "storage/page/tuple.h"


namespace YourSQL {
    class Executor {
    public:
        explicit Executor(std::shared_ptr<ExecutorContext> context,PhysicalOperatorTypes type) : type_(type),context_(context) {
        }

        virtual ~Executor() = default;

        virtual auto Open() -> void = 0;

        virtual auto Close() -> void = 0;

        virtual auto Next(Tuple *tuple) -> bool = 0;

        PhysicalOperatorTypes type_;
        std::shared_ptr<ExecutorContext> context_;
        std::vector<std::unique_ptr<Executor>> children_;
    };
}
