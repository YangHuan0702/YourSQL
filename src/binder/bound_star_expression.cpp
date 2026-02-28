//
// Created by 杨欢 on 2026/2/28.
//
#include "binder/binder.h"
using namespace YourSQL;

auto Binder::BoundStarExpression(std::string &table_name) -> std::unique_ptr<YourSQL::BoundStarExpression> {
    if (catalog_->table_name_idx_.find(table_name) == catalog_->table_name_idx_.end()) {
        throw std::runtime_error("["+table_name+"] don`t exist in the catalog.");
    }

    const entry_id table_id = catalog_->table_name_idx_[table_name];
    const auto &table_entry = catalog_->tables_[table_id];

    auto bound_star_expression = std::make_unique<class BoundStarExpression>();
    for (const auto &column_entry : table_entry->columns_) {
        bound_star_expression->columns_.push_back(BoundColumnRefExpression(table_name,column_entry->name_));
    }
    return bound_star_expression;
}
