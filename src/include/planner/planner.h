//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include <memory>

#include "binder/statement/bound_select_statement.h"
#include "logical/logical_operator.h"

namespace YourSQL {
    class Planner {
    public:
        explicit Planner() ;
        ~Planner() = default;

        auto CreateLogicalPlan(std::unique_ptr<BoundStatement> statement) -> std::unique_ptr<LogicalOperator>;

    private:
        auto LogicalSelectPlan(std::unique_ptr<BoundSelectStatement> select_statement) -> std::unique_ptr<LogicalOperator>;
    };
}
