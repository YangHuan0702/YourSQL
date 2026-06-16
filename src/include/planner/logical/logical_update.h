//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>
#include <vector>

#include "logical_operator.h"
#include "binder/bound_expression.h"
#include "binder/statement/bound_update_statement.h"
#include "common/type.h"

namespace YourSQL {
    class LogicalUpdate : public LogicalOperator {
    public:
        explicit LogicalUpdate(entry_id table_id, std::vector<BoundUpdateSet> set_clauses,
                               std::unique_ptr<BoundExpression> where_expr)
            : LogicalOperator(LogicalOperatorType::LOGICAL_UPDATE),
              table_id_(table_id), set_clauses_(std::move(set_clauses)),
              where_expr_(std::move(where_expr)) {
        }

        ~LogicalUpdate() override = default;

        auto to_string() -> std::string override {
            return "LogicalUpdate";
        }

        entry_id table_id_;
        std::vector<BoundUpdateSet> set_clauses_;
        std::unique_ptr<BoundExpression> where_expr_;
    };
}
