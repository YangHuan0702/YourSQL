//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include "buffer/page.h"
#include "common/types/index_page_type.h"

namespace YourSQL {

    struct IndexPageHeader {
        IndexPageType type_;
        uint16_t size_;
        uint16_t max_size_;
        page_id_t page_id_;
        page_id_t parent_page_id_;
    };

    class IndexPage {
    public:
        explicit IndexPage(Page *page) : page_(page) {}

        auto Init(IndexPageType type, page_id_t page_id, page_id_t parent_id, uint16_t max_size) -> void;
        auto IsLeafPage() const -> bool;
        auto GetSize() const -> int;
        auto SetSize(int size) -> void;
        auto IncreaseSize(int delta) -> void;
        auto GetMaxSize() const -> int;
        auto GetMinSize() const -> int;
        auto GetPageId() const -> page_id_t;
        auto GetParentPageId() const -> page_id_t;
        auto SetParentPageId(page_id_t parent_id) -> void;

    protected:
        auto Header() -> IndexPageHeader *;

        Page *page_;
    };

}
