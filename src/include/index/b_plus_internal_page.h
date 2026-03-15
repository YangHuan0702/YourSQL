//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include <utility>

#include "index_page.h"

namespace YourSQL {

    template <class KeyType>
    class BPlusInternalPage : public IndexPage {
    public:
        using MappingType = std::pair<KeyType,page_id_t>;
        explicit BPlusInternalPage(Page *page) : IndexPage(page) {}
        ~BPlusInternalPage() = default;

        auto Init(page_id_t page_id, page_id_t parent_id, int max_size)  -> void;
        auto KeyAt(int index) const -> const KeyType &;
        auto ValueAt(int index) const -> page_id_t;
        auto SetKeyAt(int index, const KeyType &key) -> void;
        auto SetValueAt(int index, page_id_t child) -> void;

        auto LookupChild(const KeyType &key) const -> page_id_t;
        auto InsertAfter(page_id_t old_child, const KeyType &middle_key, page_id_t new_child) -> void;
        auto Remove(int index) -> void;

    private:
        auto Array() -> MappingType *;
    };

}
