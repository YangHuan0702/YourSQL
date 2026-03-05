//
// Created by huan.yang on 2026-03-04.
//
#include "planner/planner.h"
#include "planner/logical/logical_seq_scan.h"
#include "planner/physical/physical_seq_scan.h"

using namespace YourSQL;

auto Planner::PhysicalTransformerGet(std::unique_ptr<LogicalOperator> &logical_operator) -> std::unique_ptr<PhysicalOperator> {
    auto *seq_scan = dynamic_cast<LogicalSeqScan*>(logical_operator.get());
    auto physical_operator = std::make_unique<PhysicalSeqScan>(seq_scan->table_id_);
    for (auto &operator_ : seq_scan->children_) {
        auto phy_children = CreatePhysicalPlan(std::move(operator_));
        physical_operator->children_.push_back(std::move(physical_operator));
    }
    return physical_operator;
}
