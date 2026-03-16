//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include "buffer/page.h"
#include "common/types/index_page_type.h"

namespace YourSQL {
#define  INDEX_HEADER_TYPE_OFFSET 0
#define INDEX_HEADER_SIZE_OFFSET 1
#define INDEX_HEADER_MAX_SIZE_OFFSET (1 + sizeof(uint16_t))
#define INDEX_HEADER_PAGE_ID_OFFSET (1 + sizeof(uint16_t) * 2)
#define INDEX_HEADER_PARENT_PAGE_ID_OFFSET (1 + sizeof(uint16_t) * 2 + sizeof(page_id_t))

#define INDEX_COMMON_HEADER_SIZE sizeof(IndexPageHeader)

    /**
     * ---------------------------------------------------------------------
     * | type(1) | size（2） | max_size(2) | page_id(8) | parent_page_id(8) |
     * ---------------------------------------------------------------------
     */
    struct IndexPageHeader {
        IndexPageType type_;
        uint16_t size_;
        uint16_t max_size_;
        page_id_t page_id_;
        page_id_t parent_page_id_;
    };

    class IndexPage {
    public:
        explicit IndexPage(Page *page) : page_(page) {
        }

        auto Init(IndexPageType type, page_id_t page_id, page_id_t parent_id, uint16_t max_size) -> void;

        auto IsLeafPage() const -> bool;

        auto GetSize() const -> uint16_t;

        auto SetSize(uint16_t size) -> void;

        auto IncreaseSize(int delta) -> void;

        auto GetMaxSize() const -> uint16_t;

        auto GetMinSize() const -> uint16_t;

        auto GetPageId() const -> page_id_t;

        auto GetParentPageId() const -> page_id_t;

        auto SetParentPageId(page_id_t parent_id) -> void;

    protected:
        auto Header() -> IndexPageHeader *;

        IndexPageHeader header_{};
        Page *page_;
    };
}
