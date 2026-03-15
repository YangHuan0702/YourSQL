//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include <utility>

#include "buffer/buffer_manager.h"

namespace YourSQL {

    template <class KeyType,class ValType>
    class BPlusIndexIterator {
    public:
        using MappingType = std::pair<KeyType,ValType>;

        BPlusIndexIterator() = default;
        BPlusIndexIterator(std::shared_ptr<BufferManager> buffer_manager,page_id_t leaf_page_id,int index);
        ~BPlusIndexIterator();

        auto operator*() const -> MappingType;
        auto operator++() -> BPlusIndexIterator &;

        auto operator==(const BPlusIndexIterator &other) const -> bool;
        auto operator!=(const BPlusIndexIterator &other) const -> bool {
            return !operator==(other);
        }

        auto IsEnd() const -> bool;

    private:
        auto LoadLeaf(page_id_t leaf_page_id) -> void;
        auto ReleaseLeaf() -> void;

        std::shared_ptr<BufferManager> buffer_manager_;
        Page *page_{nullptr};
        page_id_t leaf_page_id_{};
        int index_{0};
        bool is_end_{true};
    };

}
