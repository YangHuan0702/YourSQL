//
// Created by huan.yang on 2026-03-09.
//
#include <iostream>

#include "binder/binder.h"

using namespace YourSQL;

auto Binder::BoundInsertStatement(
    std::unique_ptr<InsertStatement> parser_statement) -> std::unique_ptr<YourSQL::BoundInsertStatement> {
    auto r = std::make_unique<class BoundInsertStatement>();

    auto table_id = catalog_->table_name_idx_[parser_statement->table_name_];
    auto &table = catalog_->tables_[table_id];

    r->table_id_ = table_id;
    size_t i = 0;
    for (const auto& column_name: parser_statement->column_name_) {
        entry_id column_id = table->column_name_idx[column_name];
        auto column = table->GetColumnForId(column_id);

        r->column_ids_.push_back(column_id);
        r->values_.push_back(parser_statement->values_[i]);
        i++;
    }

    return r;
}
