//
// Created by huan.yang on 2026-03-05.
//
#pragma once
#include "physical_operator.h"
#include "planner/logical/logical_projection.h"
#include "binder/bound_column_ref_expression.h"
#include "expression/physical_column_expression.h"

namespace YourSQL {
    class PhysicalProjection : public PhysicalOperator {
    public:
        PhysicalProjection() : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_PROJECTION) {}
        explicit PhysicalProjection(std::unique_ptr<LogicalProjection> logical_projection) : PhysicalOperator(
            PhysicalOperatorTypes::PHYSICAL_PROJECTION) {
            for (auto &bound_expression: logical_projection->expressions_) {
                BoundColumnRefExpression *expr = dynamic_cast<BoundColumnRefExpression *>(bound_expression.release());
                columns_.push_back(expr->column_id_);
            }
        }

        ~PhysicalProjection() override = default;

        auto to_string() -> std::string override {
            return "";
        }

        std::vector<entry_id> columns_;
    };
}
