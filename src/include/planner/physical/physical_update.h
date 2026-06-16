//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>
#include <vector>

#include "physical_operator.h"
#include "physical/expression/physical_expression.h"
#include "binder/statement/bound_update_statement.h"
#include "common/type.h"

namespace YourSQL {
    class PhysicalUpdate : public PhysicalOperator {
    public:
        explicit PhysicalUpdate(entry_id table_id, std::vector<BoundUpdateSet> set_clauses,
                                std::unique_ptr<PhysicalExpression> filter)
            : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_UPDATE),
              table_id_(table_id), set_clauses_(std::move(set_clauses)), filter_(std::move(filter)) {
        }

        ~PhysicalUpdate() override = default;

        auto to_string() -> std::string override {
            return "PhysicalUpdate";
        }

        entry_id table_id_;
        std::vector<BoundUpdateSet> set_clauses_;       // column_id -> 新值
        std::unique_ptr<PhysicalExpression> filter_;    // 可为空（无 WHERE）
    };
}
