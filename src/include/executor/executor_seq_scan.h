//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "executor.h"
#include "storage/page/tuple.h"

namespace YourSQL {
    class ExecutorSeqScan : public Executor {
    public:
        explicit ExecutorSeqScan(std::shared_ptr<ExecutorContext> context,entry_id table_id) :
        Executor(context,PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN),table_id_(table_id) {}
        ~ExecutorSeqScan() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        entry_id table_id_;
        size_t cursor_{};

    };
}
