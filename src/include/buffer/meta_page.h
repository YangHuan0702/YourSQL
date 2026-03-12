//
// Created by huan.yang on 2026-03-07.
//
#pragma once
#include <memory>

#include "common/type.h"
#include <string>
#include <unordered_map>


#include "buffer_manager.h"
#include "common/constant.h"

namespace YourSQL {
    struct MetaItem {
        page_id_t first_page_id;
        std::string table_name_;
        size_t num_rows_;
        entry_id table_id_;
        page_id_t last_page_id;
        size_t offset;
    };

#define ITEMS_OFFSET_BEGIN (sizeof(size_t) * 3)
#define ITEM_FIXED_SIZE (sizeof(size_t) + sizeof(page_id_t) * 2 + sizeof(size_t) + sizeof(entry_id))

    /**
     * Header format:
     *  ---------------------------------------------------------------------
     * | version | table_size | last point | item_1 | item_2 | item_3 | ... |
     *  ---------------------------------------------------------------------
     *
     *
     *  items format:
    *  ----------------------------------------------------------------------
     * | table_name_len | table_name | first id | rows | table_id | last_id |
     *  ---------------------------------------------------------------------
     */
    class MetaPage {
    public:
        explicit MetaPage(const std::shared_ptr<BufferManager>& buffer_manager) : buffer_manager_(buffer_manager) {
            meta_page_ = buffer_manager->FetchPage(META_PAGE_ID);
        }

        ~MetaPage() {
            buffer_manager_->Release(meta_page_->id_);
        }

        auto Init() -> void;

        auto ReadMata() -> void;

        auto AddTable(const MetaItem &item) -> void ;

        auto UpdateTableLastId(entry_id table_id, page_id_t last_page_id) -> void ;

        auto UpdateTableRows(entry_id table_id, size_t change_size) -> void ;

        auto GetFirstPageId(const std::string &tale_name) -> page_id_t;

        auto GetFirstPageId(entry_id table_id) -> page_id_t;

        auto GetNameLen(const MetaItem &item) -> size_t;

        auto UpdateTableSize(size_t change_size) -> void;

        size_t version_{};
        size_t table_size_{};
        size_t last_point_{};

        std::shared_ptr<BufferManager> buffer_manager_;
        Page *meta_page_{nullptr};
        std::unordered_map<std::string, page_id_t> name_tables_;
        std::unordered_map<entry_id, page_id_t> id_tables_;
        std::unordered_map<entry_id, MetaItem> items_{};
    };
}
