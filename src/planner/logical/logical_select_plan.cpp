//
// Created by huan.yang on 2026-03-02.
//
#include "planner/planner.h"
#include "planner/logical/logical_filter.h"
#include "planner/logical/logical_projection.h"
#include "planner/logical/logical_seq_scan.h"

using namespace YourSQL;


auto Planner::LogicalSelectPlan(std::unique_ptr<BoundSelectStatement> select_statement) -> std::unique_ptr<LogicalOperator> {
    /**
     * Scan -> Filter -> agg -> projection -> limit -> grouo by
     */
    auto root = std::make_unique<LogicalSeqScan>(select_statement->table_->table_id_);

    // filter
    if (select_statement->where_expr_) {
        auto filter = std::make_unique<LogicalFilter>(std::move(select_statement->where_expr_->children_));
        filter->children_.push_back(std::move(root));
        root = std::move(filter);
    }

    // projection
    if (select_statement->select_) {
        auto projection = std::make_unique<LogicalProjection>(std::move(select_statement->select_->children_));
        projection->children_.push_back(std::move(root));
        root = std::move(projection);
    }
    return root;
}
