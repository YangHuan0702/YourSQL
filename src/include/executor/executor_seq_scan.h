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
        Schema schema_;

    private:
        // 判断写入版本的事务 id 对当前事务快照是否可见
        auto IsVisible(tx_id_t version_trx_id) -> bool;
    };
}
