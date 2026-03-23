//
// Created by huan.yang on 2026-03-23.
//
#include "planner/planner.h"
#include "planner/logical/logical_create_table.h"

using namespace YourSQL;

auto Planner::LogicalCreateTablePlan(std::unique_ptr<BoundCreateTableStatement> bound_create_table_statement) -> std::unique_ptr<LogicalOperator> {
    return std::make_unique<LogicalCreateTable>(bound_create_table_statement->table_id_);
}
