//
// Created by 杨欢 on 2026/2/28.
//
#include "binder/binder.h"

using namespace YourSQL;

auto Binder::BoundColumnRefExpression(std::string &table_name, std::string &column_name) -> std::unique_ptr<YourSQL::BoundColumnRefExpression> {
    if (catalog_->table_name_idx_.find(table_name) == catalog_->table_name_idx_.end()) {
        throw std::runtime_error("["+table_name+"] don`t exist in the catalog.");
    }
    const auto table_id = catalog_->table_name_idx_[table_name];
    const auto &table_entry = catalog_->tables_[table_id];

    if (table_entry->column_name_idx.find(column_name) == table_entry->column_name_idx.end()) {
        throw std::runtime_error("["+table_name+"."+column_name+"] don`t exist in the catalog.");
    }

    return std::make_unique<class BoundColumnRefExpression>(table_id,table_entry->column_name_idx[column_name]);
}
