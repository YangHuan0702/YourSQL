//
// Created by 杨欢 on 2026/2/28.
//
#pragma once
#include "bound_statement.h"
#include "binder/bound_table_ref_expression.h"

namespace YourSQL {

    class BoundSelectStatement : public BoundStatement {
    public:
        explicit BoundSelectStatement() : BoundStatement(StatementType::SELECT) {

        }
        ~BoundSelectStatement() override = default;

        auto to_string() -> std::string override {
            return "BoundSelectStatement";
        }

        std::vector<std::unique_ptr<BoundExpression>> select_;
        std::unique_ptr<BoundTableRefExpression> table_;
        std::unique_ptr<BoundExpression> where_expr_;
    };

}
