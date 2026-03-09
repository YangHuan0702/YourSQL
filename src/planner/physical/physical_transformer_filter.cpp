//
// Created by huan.yang on 2026-03-04.
//
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
#include "planner/physical/physical_filter.h"

using namespace YourSQL;

auto Planner::PhysicalTransformerFilter(std::unique_ptr<LogicalOperator> &logical_operator) -> std::unique_ptr<PhysicalOperator> {
    auto filter_logical_operator = dynamic_cast<LogicalFilter *>(logical_operator.release());

    auto r = std::make_unique<PhysicalFilter>();
    for (auto &operator_ : filter_logical_operator->children_) {
        r->children_.push_back(CreatePhysicalPlan(operator_));
    }
    return r;
}

