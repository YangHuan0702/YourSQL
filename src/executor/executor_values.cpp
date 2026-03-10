//
// Created by huan.yang on 2026-03-10.
//
#include "executor/executor_values.h"

using namespace YourSQL;

auto ExecutorValues::Close() -> void {
    if (row) {
        delete row;
    }
}


auto ExecutorValues::Open() -> void {
    if (context_->catalog_->tables_.find(table_id_) == context_->catalog_->tables_.end()) {
        throw std::runtime_error("ExecutorValues::Open don`t find target table.");
    }

    auto &table_entry = context_->catalog_->tables_[table_id_];

    std::unordered_map<entry_id,size_t> column_index_map;
    for (size_t i = 0; i < column_ids_.size(); ++i) {
        column_index_map[column_ids_[i]] = i;
    }
    std::vector<Value> full_values;
    for (auto column : table_entry->columns_) {
        if (column_index_map.find(column.first) == column_index_map.end()) {
            full_values.push_back(Value(true));
        } else {
            full_values.push_back(values_[column_index_map[column.first]]);
        }
    }
    Schema schema = table_entry->GetSchema();
    row = new Row(schema);
}

auto ExecutorValues::Next(Tuple *tuple) -> bool {
    tuple->data_ = row->Serialize();
    tuple->schema_ = row->schema_;
    used_ = true;
    return true;
}

