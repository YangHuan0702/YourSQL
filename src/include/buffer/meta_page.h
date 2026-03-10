//
// Created by huan.yang on 2026-03-07.
//
#pragma once
#include <memory>

#include "common/type.h"
#include <string>
#include <unordered_map>


#include "buffer_manager.h"

namespace YourSQL {
    struct MetaItem {
        page_id_t first_page_id;
        std::string table_name_;
        size_t num_rows_;
        entry_id table_id_;
        page_id_t last_page_id;
    };


    class MetaPage {
    public:
        explicit MetaPage() = default;

        ~MetaPage() = default;

        auto Init(const std::shared_ptr<BufferManager> &buffer_manager) -> void;

        auto GetFirstPageId(const std::string &tale_name) -> page_id_t;
        auto GetFirstPageId(entry_id table_id) -> page_id_t;

        size_t version_{};
        size_t table_size_{};

        Page *meta_page_{nullptr};
        std::unordered_map<std::string, page_id_t> name_tables_;
        std::unordered_map<entry_id,page_id_t> id_tables_;
        std::unordered_map<entry_id, MetaItem> items_{};
    };
}
