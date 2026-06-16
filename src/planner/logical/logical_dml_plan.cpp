//
// Created by huan.yang on 2026-03-22.
//
#include "planner/planner.h"
#include "planner/logical/logical_delete.h"
#include "planner/logical/logical_update.h"

using namespace YourSQL;

auto Planner::LogicalDeletePlan(
    std::unique_ptr<BoundDeleteStatement> delete_statement) -> std::unique_ptr<LogicalOperator> {
    return std::make_unique<LogicalDelete>(delete_statement->table_id_,
                                           std::move(delete_statement->where_expr_));
}

auto Planner::LogicalUpdatePlan(
    std::unique_ptr<BoundUpdateStatement> update_statement) -> std::unique_ptr<LogicalOperator> {
    return std::make_unique<LogicalUpdate>(update_statement->table_id_,
                                           std::move(update_statement->set_clauses_),
                                           std::move(update_statement->where_expr_));
}
