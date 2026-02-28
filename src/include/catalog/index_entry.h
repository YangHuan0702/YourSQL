//
// Created by 杨欢 on 2026/2/28.
//

#pragma once
#include <unordered_set>

#include "base_entry.h"

namespace YourSQL {

    class IndexEntry : public BaseEntry {
    public:
        explicit IndexEntry(entry_id index_id,std::string &index_name) : BaseEntry(index_id,index_name) {}
        ~IndexEntry() override = default;

        auto to_string() -> std::string override {
            return name_;
        }

        std::unordered_set<entry_id> column_ids;
    };

}
