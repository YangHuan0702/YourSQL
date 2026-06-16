//
// Created by huan.yang on 2026-03-20.
//
#pragma once
#include <memory>
#include <string>

#include "executor.h"
#include "buffer/table_iterator.h"
#include "planner/physical/expression/physical_expression.h"

namespace YourSQL {
    /**
     * MVCC 删除执行器。
     *
     * 由于当前火山模型下子执行器只返回 Tuple 数据、不返回 RID，
     * ExecutorDelete 自行持有 TableIterator 遍历目标表，对每条
     * 对当前事务可见、未被删除、且满足 WHERE 条件的记录执行标记删除：
     *   1. 读取旧版本记录，写入 undo 日志，得到 roll_ptr
     *   2. 在原页上 MarkDelete（写 xmax=当前事务、roll_ptr、删除标记）
     */
    class ExecutorDelete : public Executor {
    public:
        explicit ExecutorDelete(std::shared_ptr<ExecutorContext> context, std::string table_name,
                                std::unique_ptr<PhysicalExpression> filter = nullptr)
            : Executor(context, PhysicalOperatorTypes::PHYSICAL_DELETE),
              table_name_(std::move(table_name)), filter_(std::move(filter)) {
        }

        ~ExecutorDelete() override = default;

        auto Open() -> void override;
        auto Close() -> void override;
        auto Next(Tuple *tuple) -> bool override;

        // 已删除的行数
        auto GetDeletedCount() const -> size_t { return deleted_count_; }

    private:
        auto IsVisible(tx_id_t version_trx_id) -> bool;

        std::string table_name_;
        std::unique_ptr<PhysicalExpression> filter_;  // WHERE 条件，可为空
        TableIterator *iterator_{nullptr};
        Schema schema_;
        entry_id table_id_{};
        size_t deleted_count_{0};
        bool done_{false};
    };
}

