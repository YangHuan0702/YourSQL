//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_factory.h"

#include "executor/executor_filter.h"
#include "executor/executor_projection.h"
#include "executor/executor_seq_scan.h"
#include "executor/executor_values.h"
#include "executor/executor_insert.h"
#include "planner/physical/physical_filter.h"
#include "planner/physical/physical_projection.h"
#include "planner/physical/physical_seq_scan.h"
#include "planner/physical/physical_values.h"
#include "planner/physical/physical_insert.h"

using namespace YourSQL;


auto ExecutorFactory::BuildExecutor(std::unique_ptr<PhysicalOperator> &physical_operator) -> std::unique_ptr<Executor> {
    switch (physical_operator->type_) {
        case PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN: {
            auto seq_scan = dynamic_cast<PhysicalSeqScan *>(physical_operator.get());
            return std::make_unique<ExecutorSeqScan>(context_, context_->catalog_->GetTableName(seq_scan->table_id_));
        }
        case PhysicalOperatorTypes::PHYSICAL_PROJECTION: {
            auto projection = dynamic_cast<PhysicalProjection *>(physical_operator.get());
            return std::make_unique<ExecutorProjection>(context_, projection->columns_);
        }
        case PhysicalOperatorTypes::PHYSICAL_FILTER: {
            auto filter = dynamic_cast<PhysicalFilter *>(physical_operator.get());
            auto executor_filter = std::make_unique<ExecutorFilter>(context_, std::move(filter->expressions_));
            for (auto &operator_: filter->children_) {
                executor_filter->children_.push_back(BuildExecutor(operator_));
            }
            return executor_filter;
        }
        case PhysicalOperatorTypes::PHYSICAL_VALUES: {
            auto values = dynamic_cast<PhysicalValues *>(physical_operator.get());
            if (context_->catalog_->tables_.find(table_id_) == context_->catalog_->tables_.end()) {
                throw std::runtime_error("ExecutorFactory::BuildExecutor Invalid table id");
            }
            auto &table_entry = context_->catalog_->tables_[table_id_];
            return std::make_unique<ExecutorValues>(context_, values->values_,table_entry->GetColumnIds(),table_id_);
        }
        case PhysicalOperatorTypes::PHYSICAL_INSERT: {
            auto insert = dynamic_cast<PhysicalInsert *>(physical_operator.get());
            auto executor_insert = std::make_unique<ExecutorInsert>(context_, insert->table_id_, insert->column_ids_);
            for (auto &operator_: insert->children_) {
                executor_insert->children_.push_back(BuildExecutor(operator_));
            }
            return executor_insert;
        }
        default:
            throw std::runtime_error("Unsupported physical operator type");
    }
}
