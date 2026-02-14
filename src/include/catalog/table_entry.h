//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "base_entry.h"
#include "column_entry.h"

namespace YourSQL {

    class TableEntry : public BaseEntry {
    public:
        explicit TableEntry(entry_id id, std::string &name);
        ~TableEntry() override = default;

        auto to_string() -> std::string override;

        std::unordered_map<std::string,size_t> column_map_;
        std::vector<std::unique_ptr<ColumnEntry>> columns_;
    };

}
