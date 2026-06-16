//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "executor.h"
#include "buffer/table_iterator.h"
#include "binder/statement/bound_update_statement.h"
#include "planner/physical/expression/physical_expression.h"

namespace YourSQL {
    /**
     * MVCC 更新执行器：UPDATE = delete-old + insert-new（绝不原地覆盖）。
     *
     * 为避免 Halloween problem（边扫描边插入导致新行被重复更新），
     * 分两阶段：
     *   阶段一：遍历表，收集所有可见、未删除、满足 WHERE 的行（RID + 新行字节）
     *   阶段二：对每个命中行，旧版本入 undo + MarkDelete 旧行，再插入携带
     *           新值与 roll_ptr 的新版本，形成版本链
     */
    class ExecutorUpdate : public Executor {
    public:
        explicit ExecutorUpdate(std::shared_ptr<ExecutorContext> context, entry_id table_id,
                                std::vector<BoundUpdateSet> set_clauses,
                                std::unique_ptr<PhysicalExpression> filter = nullptr)
            : Executor(context, PhysicalOperatorTypes::PHYSICAL_UPDATE),
              table_id_(table_id), set_clauses_(std::move(set_clauses)), filter_(std::move(filter)) {
        }

        ~ExecutorUpdate() override = default;

        auto Open() -> void override;
        auto Close() -> void override;
        auto Next(Tuple *tuple) -> bool override;

        auto GetUpdatedCount() const -> size_t { return updated_count_; }

    private:
        auto IsVisible(tx_id_t version_trx_id) -> bool;

        entry_id table_id_;
        std::vector<BoundUpdateSet> set_clauses_;
        std::unique_ptr<PhysicalExpression> filter_;
        TableIterator *iterator_{nullptr};
        Schema schema_;
        std::string table_name_;
        size_t updated_count_{0};
        bool done_{false};
    };
}
