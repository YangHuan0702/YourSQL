//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "catalog/base_entry.h"
#include "catalog/column_entry.h"
#include "catalog/index_entry.h"
#include "common/util/IdUtil.h"

namespace YourSQL {

    class TableEntry : public BaseEntry {
    public:
        explicit TableEntry(entry_id id, std::string &name);
        ~TableEntry() override = default;

        auto to_string() -> std::string override;

        std::mutex lock;

        auto AddColumn(std::unique_ptr<ColumnEntry> column_entry) -> void {
            column_name_idx[column_entry->name_] = column_entry->id_;
            columns_[column_entry->id_] = std::move(column_entry);
        }

        std::unique_ptr<IndexEntry> index_entry;
        std::unordered_map<std::string,entry_id> column_name_idx;
        std::unordered_map<entry_id,std::unique_ptr<ColumnEntry>> columns_;
    };

}
