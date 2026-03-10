//
// Created by huan.yang on 2026-03-10.
//
#include "planner/planner.h"
#include "planner/logical/logical_insert.h"
#include "planner/logical/logical_seq_scan.h"
#include "planner/logical/logical_values.h"

using namespace YourSQL;

auto Planner::LogicalInsertPlan(
    std::unique_ptr<BoundInsertStatement> insert_statement) -> std::unique_ptr<LogicalOperator> {
    /**
    *  LogicalInsert
    *        |
    *   LogicalValues
    **/
    std::unique_ptr<LogicalOperator> values = std::make_unique<LogicalValues>(insert_statement->values_);
    values->SetLogicalOpType(LogicalOperatorType::LOGICAL_VALUES);

    auto logical_insert = std::make_unique<LogicalInsert>(insert_statement->table_id_,insert_statement->column_ids_);
    logical_insert->children_.push_back(std::move(values));
    return logical_insert;
}
