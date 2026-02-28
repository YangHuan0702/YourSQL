//
// Created by 杨欢 on 2026/2/28.
//
#include "binder/binder.h"

using namespace YourSQL;

auto Binder::BoundTableRefExpression(std::string &table_name) -> std::unique_ptr<class BoundTableRefExpression> {
    if (catalog_->table_name_idx_.find(table_name) == catalog_->table_name_idx_.end()) {
        throw std::runtime_error("["+table_name+"] don`t exist in the catalog.");
    }
    return std::make_unique<class BoundTableRefExpression>(catalog_->table_name_idx_[table_name]);
}
