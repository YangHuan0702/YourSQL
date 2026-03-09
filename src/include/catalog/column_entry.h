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
        explicit ColumnEntry(entry_id id, std::string name,ColumnTypes type);
        ~ColumnEntry() override = default;
        ColumnEntry(const ColumnEntry &copy) : BaseEntry(copy.id_,copy.name_) {
            column_types = copy.column_types;
            max_size_ = copy.max_size_;
            default_value_ = copy.default_value_;
        }
        auto operator=(const ColumnEntry &copy) -> ColumnEntry & {
            this->id_ = copy.id_;
            this->name_ = copy.name_;
            this->column_types = copy.column_types;
            this->max_size_ = copy.max_size_;
            this->default_value_ = copy.default_value_;
            return *this;
        }

        auto to_string() -> std::string override;

        ColumnTypes column_types{};
        size_t max_size_{};
        Value default_value_{};
    };

}