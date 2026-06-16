//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>

#include "logical_operator.h"
#include "binder/bound_expression.h"
#include "common/type.h"

namespace YourSQL {
    class LogicalDelete : public LogicalOperator {
    public:
        explicit LogicalDelete(entry_id table_id, std::unique_ptr<BoundExpression> where_expr)
            : LogicalOperator(LogicalOperatorType::LOGICAL_DELETE),
              table_id_(table_id), where_expr_(std::move(where_expr)) {
        }

        ~LogicalDelete() override = default;

        auto to_string() -> std::string override {
            return "LogicalDelete";
        }

        entry_id table_id_;
        std::unique_ptr<BoundExpression> where_expr_;
    };
}
