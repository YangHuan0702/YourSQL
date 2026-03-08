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

        MetaItem item;
        item.table_name_ = table_name;
        item.first_page_id = first_id;
        item.num_rows_ = rows;

        table_[table_name] = first_id;
        items_[table_name] = item;
    }
}


auto MetaPage::GetFirstPageId(const std::string &table_name) -> page_id_t {
    if (table_.find(table_name) == table_.end()) {
        throw std::runtime_error("MetaPage does not exist");
    }
    return table_[table_name];
}


