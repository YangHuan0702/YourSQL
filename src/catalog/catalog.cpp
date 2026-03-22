//
// Created by 杨欢 on 2026/2/14.
//
#include "catalog/catalog.h"
using namespace YourSQL;


auto Catalog::InitFromMetaPage() -> void {
    if (meta_page_ == nullptr) return;


    for (auto &[id,table] : meta_page_->items_) {
        table_name_idx_[table.table_name_] = table.table_id_;

        auto table_entry = std::make_unique<TableEntry>(table.table_id_,table.table_name_);

        for (const auto& item : table.items_) {
            ColumnEntry column_entry(item.column_id_,item.column_name_,item.type_);
            table_entry->AddColumn(column_entry);
        }

        tables_[table.table_id_] = std::move(table_entry);
    }
}

