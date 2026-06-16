//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include "bound_statement.h"
#include "binder/bound_table_ref_expression.h"

namespace YourSQL {

    class BoundDeleteStatement : public BoundStatement {
    public:
        explicit BoundDeleteStatement() : BoundStatement(StatementType::DELETE) {}
        ~BoundDeleteStatement() override = default;

        auto to_string() -> std::string override {
            return "BoundDeleteStatement";
        }

        entry_id table_id_{};
        std::unique_ptr<BoundTableRefExpression> table_;
        std::unique_ptr<BoundExpression> where_expr_;
    };

}
