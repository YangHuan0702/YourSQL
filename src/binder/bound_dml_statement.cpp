//
// Created by huan.yang on 2026-03-22.
//
#include "binder/binder.h"

using namespace YourSQL;

auto Binder::BoundDeleteStatement(
    std::unique_ptr<DeleteStatement> parser_statement) -> std::unique_ptr<YourSQL::BoundDeleteStatement> {
    auto ans = std::make_unique<class BoundDeleteStatement>();

    if (catalog_->table_name_idx_.find(parser_statement->table_name_) == catalog_->table_name_idx_.end()) {
        throw std::runtime_error("Binder::BoundDeleteStatement table not found: " + parser_statement->table_name_);
    }
    auto table_id = catalog_->table_name_idx_[parser_statement->table_name_];
    ans->table_id_ = table_id;
    ans->table_ = BoundTableRefExpression(parser_statement->table_name_);

    // where（可为空）
    ans->where_expr_ = BoundCompExpression(parser_statement->where_expression_, ans->table_);
    return ans;
}

auto Binder::BoundUpdateStatement(
    std::unique_ptr<UpdateStatement> parser_statement) -> std::unique_ptr<YourSQL::BoundUpdateStatement> {
    auto ans = std::make_unique<class BoundUpdateStatement>();

    if (catalog_->table_name_idx_.find(parser_statement->table_name_) == catalog_->table_name_idx_.end()) {
        throw std::runtime_error("Binder::BoundUpdateStatement table not found: " + parser_statement->table_name_);
    }
    auto table_id = catalog_->table_name_idx_[parser_statement->table_name_];
    auto &table = catalog_->tables_[table_id];

    ans->table_id_ = table_id;
    ans->table_ = BoundTableRefExpression(parser_statement->table_name_);

    // SET 子句：列名 -> column_id
    for (const auto &set_clause : parser_statement->set_clauses_) {
        if (table->column_name_idx.find(set_clause.column_name_) == table->column_name_idx.end()) {
            throw std::runtime_error("Binder::BoundUpdateStatement column not found: " + set_clause.column_name_);
        }
        BoundUpdateSet bound_set;
        bound_set.column_id_ = table->column_name_idx[set_clause.column_name_];
        bound_set.value_ = set_clause.value_;
        ans->set_clauses_.push_back(std::move(bound_set));
    }

    // where（可为空）
    ans->where_expr_ = BoundCompExpression(parser_statement->where_expression_, ans->table_);
    return ans;
}
