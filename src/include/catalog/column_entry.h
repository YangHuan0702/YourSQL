//
// Created by 杨欢 on 2026/2/13.
//

#pragma once
#include <string>

#include "base_entry.h"
#include "common/types/column_types.h"

namespace YourSQL {

    class ColumnEntry :public BaseEntry {
    public:
        explicit ColumnEntry(entry_id id, std::string &name,ColumnTypes type);
        ~ColumnEntry() override = default;

        auto to_string() -> std::string override;

        Value default_value_;
        ColumnTypes column_types;
        size_t max_size_;
    };

}