//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include <utility>

#include "index_page.h"

namespace YourSQL {

    struct LeafExtraHeader {
        page_id_t next_page_id_;
    };

    /**
     * | IndexPageHeader | LeafExtraHeader | MappingType array... |
     * @tparam KeyType key type
     * @tparam ValType val type
     */
    template <class KeyType,class ValType>
    class BPlusLeafPage : public IndexPage {
    public:
        using MappingType = std::pair<KeyType,ValType>;

        explicit BPlusLeafPage(Page *page) : IndexPage(page) {}

        auto Init(page_id_t page_id, page_id_t parent_id, int max_size) -> void;
        static auto GetMaxSize() -> size_t {
            return (PAGE_SIZE - sizeof(IndexPageHeader) - sizeof(LeafExtraHeader)) / sizeof(MappingType);
        }
        auto KeyAt(int index) const -> const KeyType &;
        auto ValueAt(int index) const -> const ValType &;
        auto GetNextPageId() const -> page_id_t;
        auto SetNextPageId(page_id_t next_page_id) -> void;

        auto LowerBound(const KeyType &key) const -> int;
        auto Insert(const KeyType &key, const ValType &value) -> bool;
        auto Lookup(const KeyType &key, std::vector<ValType> &result) const -> bool;
        auto Remove(const KeyType &key, const ValType &value) -> int;

    private:
        auto Array() -> MappingType*;

    };

}
