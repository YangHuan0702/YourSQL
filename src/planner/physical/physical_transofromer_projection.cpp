//
// Created by huan.yang on 2026-03-09.
//
#include "planner/planner.h"
#include "planner/logical/logical_projection.h"
#include "planner/physical/physical_projection.h"

using namespace YourSQL;

auto Planner::PhysicalTransformerProjection(std::unique_ptr<LogicalOperator> &logical_operator) -> std::unique_ptr<PhysicalOperator> {
    auto logical_projection = dynamic_cast<LogicalProjection*>(logical_operator.release());
    auto r = std::make_unique<PhysicalProjection>();
    for (auto &bound_expression : logical_projection->expressions_) {
        auto column_ref = dynamic_cast<BoundColumnRefExpression*>(bound_expression.release());
        r->columns_.push_back(column_ref->column_id_);
    }
    return r;
}


