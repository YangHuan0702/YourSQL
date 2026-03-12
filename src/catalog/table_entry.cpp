//
// Created by 杨欢 on 2026/2/14.
//
#include "catalog/table_entry.h"
#include "buffer/table_iterator.h"
#include "storage/page/tuple.h"

using namespace YourSQL;

TableEntry::TableEntry(entry_id id, std::string &name) : BaseEntry(id,name) {
}

auto TableEntry::to_string() -> std::string {
    return name_;
}

auto TableEntry::begin(std::shared_ptr<BufferManager> buffer_manager,
                       std::shared_ptr<MetaPage> meta_page) -> TableIterator {
    // 构建 Schema
    Schema schema;
    for (auto &pair : columns_) {
        schema.columns_.push_back(pair.second);
    }

    return TableIterator(std::move(buffer_manager), std::move(meta_page), name_, id_,schema);
}

auto TableEntry::end() -> TableIterator {
    return TableIterator();  // 返回 end 迭代器
}
