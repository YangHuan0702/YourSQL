//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <vector>

#include "bound_statement.h"
#include "binder/bound_table_ref_expression.h"
#include "parser/statement/update_statement.h"

namespace YourSQL {

    // 绑定后的一条 SET：列 id + 新值
    struct BoundUpdateSet {
        entry_id column_id_;
        Value value_;
    };

    class BoundUpdateStatement : public BoundStatement {
    public:
        explicit BoundUpdateStatement() : BoundStatement(StatementType::UPDATE) {}
        ~BoundUpdateStatement() override = default;

        auto to_string() -> std::string override {
            return "BoundUpdateStatement";
        }

        entry_id table_id_{};
        std::unique_ptr<BoundTableRefExpression> table_;
        std::vector<BoundUpdateSet> set_clauses_;
        std::unique_ptr<BoundExpression> where_expr_;
    };

}
