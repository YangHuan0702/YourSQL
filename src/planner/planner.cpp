//
// Created by huan.yang on 2026-03-02.
//
#include "planner/planner.h"

#include <sys/stat.h>

using namespace YourSQL;

auto Planner::CreateLogicalPlan(std::unique_ptr<BoundStatement> statement) -> std::unique_ptr<LogicalOperator> {
    switch (statement->type_) {
        case StatementType::SELECT: return LogicalSelectPlan(std::unique_ptr<BoundSelectStatement>(dynamic_cast<BoundSelectStatement *>(statement.release())));
        default: throw std::runtime_error("[LogicalPlanner] unknow statement type.");
    }
}
