//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

#include "buffer/table_iterator.h"
#include "catalog/base_entry.h"
#include "catalog/column_entry.h"
#include "catalog/index_entry.h"
#include "common/util/IdUtil.h"
#include "storage/page/tuple.h"

namespace YourSQL {

    class TableEntry : public BaseEntry {
    public:
        explicit TableEntry(entry_id id, std::string &name);
        ~TableEntry() override = default;

        auto to_string() -> std::string override;

        std::mutex lock;

        auto AddColumn(const ColumnEntry& column_entry) -> void {
            column_name_idx[column_entry.name_] = column_entry.id_;
            columns_.emplace(column_entry.id_,column_entry);
        }

        auto GetColumnForId(entry_id column_id) -> ColumnEntry {
            if (columns_.find(column_id) == columns_.end()) {
                throw std::runtime_error("Column not found");
            }

            return columns_.at(column_id);
        }

        auto GetSchema() -> Schema {
            Schema schema;
            for (auto &pair : columns_) {
                schema.columns_.push_back(pair.second);
            }
            return schema;
        }


        // 迭代器支持
        auto begin(std::shared_ptr<BufferManager> buffer_manager,
                   std::shared_ptr<MetaPage> meta_page) -> TableIterator;
        auto end() -> TableIterator;

        std::unique_ptr<IndexEntry> index_entry;
        std::unordered_map<std::string,entry_id> column_name_idx;
        std::unordered_map<entry_id,ColumnEntry> columns_;
    };

}
