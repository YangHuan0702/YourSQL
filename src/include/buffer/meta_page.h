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
    };


    class MetaPage {
    public:
        explicit MetaPage() = default;

        ~MetaPage() = default;

        auto Init(const std::shared_ptr<BufferManager> &buffer_manager) -> void;

        auto GetFirstPageId(const std::string &tale_name) -> page_id_t;

        size_t version_{};
        size_t table_size_{};

        std::unordered_map<std::string, page_id_t> table_;
        std::unordered_map<std::string, MetaItem> items_{};
    };
}
