//
// Created by 杨欢 on 2026/2/14.
//
#include <stdexcept>
#include "catalog/column_entry.h"

using namespace YourSQL;


ColumnEntry::ColumnEntry(entry_id id, std::string name, ColumnTypes type) : BaseEntry(id,name) {
    column_types = type;
    switch (type) {
        case ColumnTypes::BOOL: default_value_ = Value(false); break;
        case ColumnTypes::TIMESTAMP:
        case ColumnTypes::INTEGER: default_value_ = Value(0); break;
        case ColumnTypes::VARCHAR:
        case ColumnTypes::VARCHAR2:
            max_size_ = 0;
            default_value_ = Value("");
            break;
        default: throw std::runtime_error("unknow column type.");
    }
    max_size_ = 0;
}

auto ColumnEntry::to_string() -> std::string {
    return name_;
}
