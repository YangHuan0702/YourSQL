//
// Created by 杨欢 on 2026/2/14.
//

#pragma once

#include <unordered_map>
#include <memory>
#include "common/type.h"
#include "table_entry.h"
#include "common/util/IdUtil.h"

namespace YourSQL {

    class Catalog {
    public:
        explicit Catalog() = default;
        ~Catalog() = default;

        auto AddTable(std::unique_ptr<TableEntry> table_entry) -> void {
            table_name_idx_[table_entry->name_] = table_entry->id_;
            tables_[table_entry->id_] = std::move(table_entry);
        }

        std::unordered_map<std::string,entry_id> table_name_idx_;
        std::unordered_map<entry_id,std::unique_ptr<TableEntry>> tables_;
    };

}
