//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_factory.h"

#include "executor/executor_filter.h"
#include "executor/executor_projection.h"
#include "executor/executor_seq_scan.h"
#include "planner/physical/physical_filter.h"
#include "planner/physical/physical_projection.h"
#include "planner/physical/physical_seq_scan.h"

using namespace YourSQL;


auto ExecutorFactory::BuildExecutor(std::unique_ptr<PhysicalOperator> &physical_operator) -> std::unique_ptr<Executor> {
    switch (physical_operator->type_) {
        case PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN: {
            auto seq_scan = dynamic_cast<PhysicalSeqScan*>(physical_operator.get());
            return std::make_unique<ExecutorSeqScan>(context_,context_->catalog_->GetTableName(seq_scan->table_id_));
        }
        case PhysicalOperatorTypes::PHYSICAL_PROJECTION: {
            auto projection = dynamic_cast<PhysicalProjection*>(physical_operator.get());
            return std::make_unique<ExecutorProjection>(context_,projection->columns_);
        }
        case PhysicalOperatorTypes::PHYSICAL_FILTER: {
            auto filter = dynamic_cast<PhysicalFilter*>(physical_operator.get());
            auto executor_filter = std::make_unique<ExecutorFilter>(context_,std::move(filter->expressions_));
            for (auto &operator_ : filter->children_) {
                executor_filter->children_.push_back(BuildExecutor(operator_));
            }
            return executor_filter;
        }
        default:
            throw std::runtime_error("Unsupported physical operator type");
    }
}
