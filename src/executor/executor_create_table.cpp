//
// Created by huan.yang on 2026-03-23.
//
#include "executor/executor_create_table.h"
using namespace YourSQL;


auto ExecutorCreateTable::Open() -> void {

}


auto ExecutorCreateTable::Next(Tuple *tuple) -> bool {

    if (context_->catalog_->tables_.find(table_id_) == context_->catalog_->tables_.end()) {
        throw std::runtime_error("ExecutorCreateTable::Next create of table don`t exist in the catalog.");
    }

    auto &table_entry = context_->catalog_->tables_[table_id_];

    MetaItem meta_item;
    meta_item.table_id_ = table_entry->id_;
    meta_item.table_name_ = table_entry->name_;
    meta_item.column_size_ = table_entry->columns_.size();
    meta_item.first_page_id = 0;
    meta_item.last_page_id = 0;
    meta_item.num_rows_ = 0;

    for (auto &[id,column] : table_entry->columns_) {
        MetaColumnItem meta_column_item;
        meta_column_item.column_id_ = column.id_;
        meta_column_item.column_name_ = column.name_;
        meta_column_item.flags_ = 0;
        meta_column_item.type_ = column.column_types;
        meta_item.items_.push_back(meta_column_item);
    }

    context_->meta_page_->AddTable(meta_item);
    return true;
}


auto ExecutorCreateTable::Close() -> void {

}
