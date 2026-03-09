//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "executor.h"
#include "buffer/table_iterator.h"
#include "storage/page/tuple.h"

namespace YourSQL {
    class ExecutorSeqScan : public Executor {
    public:
        explicit
        ExecutorSeqScan(std::shared_ptr<ExecutorContext> context, std::string table_name) : Executor(context,
                PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN), table_name_(table_name), iterator_(nullptr) {
        }

        ~ExecutorSeqScan() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override;

        std::string table_name_;
        size_t cursor_{};
        TableIterator *iterator_;
    };
}
