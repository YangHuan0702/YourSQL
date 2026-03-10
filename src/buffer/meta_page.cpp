//
// Created by huan.yang on 2026-03-07.
//
#include "buffer/meta_page.h"

#include "common/constant.h"

using namespace YourSQL;

auto MetaPage::Init(const std::shared_ptr<BufferManager>& buffer_manager) -> void {
    Page *meta_page = buffer_manager->FetchPage(META_PAGE_ID);
    memcpy(&version_,meta_page->data_,sizeof(size_t));
    memcpy(&table_size_,meta_page->data_ + sizeof(size_t),sizeof(size_t));

    size_t offset = sizeof(size_t) + sizeof(size_t);
    for (size_t i = 0; i < table_size_; i++) {
        uint32_t len = 0;
        memcpy(&len, meta_page->data_ + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        std::string table_name = std::string(meta_page->data_,len);
        offset += len;

        page_id_t first_id = 0;
        memcpy(&first_id, meta_page->data_ + offset, sizeof(page_id_t));
        offset += sizeof(page_id_t);

        size_t rows = 0;
        memcpy(&rows, meta_page->data_ + offset, sizeof(size_t));
        offset += sizeof(size_t);

        entry_id table_id = 0;
        memcpy(&table_id, meta_page->data_ + offset, sizeof(entry_id));
        offset += sizeof(entry_id);

        page_id_t last_page_id = 0;
        memcpy(&last_page_id, meta_page->data_ + offset, sizeof(page_id_t));
        offset += sizeof(page_id_t);

        MetaItem item;
        item.table_name_ = table_name;
        item.first_page_id = first_id;
        item.num_rows_ = rows;
        item.table_id_ = table_id;
        item.last_page_id = last_page_id;

        name_tables_[table_name] = first_id;
        id_tables_[table_id] = first_id;
        items_[table_id] = item;
    }
}


auto MetaPage::GetFirstPageId(const std::string &table_name) -> page_id_t {
    if (name_tables_.find(table_name) == name_tables_.end()) {
        throw std::runtime_error("MetaPage::GetFirstPageId for tableName does not exist");
    }
    return name_tables_[table_name];
}

auto MetaPage::GetFirstPageId(entry_id table_id) -> page_id_t {
    if (id_tables_.find(table_id) == id_tables_.end()) {
        throw std::runtime_error("MetaPage::GetFirstPageId for tableId does not exist");
    }
    return id_tables_[table_id];
}


