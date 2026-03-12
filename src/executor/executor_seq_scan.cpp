//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_seq_scan.h"

#include <algorithm>

using namespace YourSQL;

auto ExecutorSeqScan::Close() -> void {
    if (iterator_) {
        delete iterator_;
    }
}

auto ExecutorSeqScan::Next(Tuple *tuple) -> bool {
    if (!iterator_ || iterator_->IsEnd()) { return false;}
    *tuple = **iterator_;
    ++(*iterator_);
    return true;
}

auto ExecutorSeqScan::Open() -> void {
    cursor_ = 0;
    if (context_->catalog_->table_name_idx_.find(table_name_) == context_->catalog_->table_name_idx_.end()) {
        throw std::runtime_error("TableIterator::Open Table name not found");
    }
    auto table_id = context_->catalog_->table_name_idx_[table_name_];
    auto &table = context_->catalog_->tables_[table_id];
    Schema schema;
    std::transform(table->columns_.begin(),table->columns_.end(),std::back_inserter(schema.columns_),[](const auto &p) {
        return p.second;
    });
    schema.tuple_size_ = -1;
    iterator_ = new TableIterator(context_->buffer_manager_,context_->meta_page_,table_name_,table_id,schema);
}


