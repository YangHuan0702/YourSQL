//
// Created by 杨欢 on 2026/2/14.
//
#include "catalog/table_entry.h"

using namespace YourSQL;

TableEntry::TableEntry(entry_id id, std::string &name) : BaseEntry(id,name) {
}


auto TableEntry::GetNextColumnId() -> int {
    std::lock_guard guard(lock);
    return column_id++;
}

auto TableEntry::to_string() -> std::string {
    return name_;
}
