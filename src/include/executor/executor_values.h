//
// Created by huan.yang on 2026-03-10.
//
#pragma once
#include "executor.h"
#include "common/types/column_types.h"
#include "storage/page/row.h"

namespace YourSQL {
    class ExecutorValues : public Executor {
    public:
        explicit ExecutorValues(std::shared_ptr<ExecutorContext> context, std::vector<Value> values,std::vector<entry_id> column_id,entry_id table_id)
            : Executor(context, PhysicalOperatorTypes::PHYSICAL_VALUES), context_(context),table_id_(table_id),column_ids_(std::move(column_id)),values_(std::move(values)) {
        }

        ~ExecutorValues() override = default;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Next(Tuple *tuple) -> bool override ;

    private:
        std::shared_ptr<ExecutorContext> context_;
        entry_id table_id_;
        std::vector<entry_id> column_ids_;
        std::vector<Value> values_;

        bool used_{false};
        Row *row{nullptr};
    };
}
