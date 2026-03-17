//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include "b_plus_internal_page.h"
#include "b_plus_leaf_page.h"
#include "index_manager.h"

#include <utility>
#include "buffer/buffer_manager.h"
#include "common/type.h"

namespace YourSQL {

    template <class KeyType,class ValType>
    class BPlusIndexManager : public IndexManager<KeyType,ValType> {
    public:
        explicit BPlusIndexManager(std::shared_ptr<BufferManager> buffer_manager) : buffer_manager_(std::move(buffer_manager)){}
        ~BPlusIndexManager() override = default;

        auto Insert(const KeyType &key, const ValType &val) -> bool override;

        auto Delete(const KeyType &key) -> bool override;

        auto Get(const KeyType &key, std::vector<ValType> &re) -> bool override;

    private:
        auto FindLeafPage(const KeyType &key,bool leaf_most) -> page_id_t;

        auto AdjustRoot(IndexPage *leaf_node) -> void;
        auto Redistribute(IndexPage *sibling,IndexPage *leaf_node,BPlusInternalPage<KeyType>* parent,bool ) -> void;
        auto Coalesce(IndexPage* sibling,IndexPage*leaf_node,BPlusInternalPage<KeyType>* parent, int target_index);
        auto HandleUnderflow(BPlusLeafPage<KeyType,ValType>* leaf_node) -> void;
        auto UpdateParentId(page_id_t,page_id_t new_parent_id) const -> void;
        auto GetParentId(page_id_t) const -> page_id_t;
        auto SplitLeaf(BPlusLeafPage<KeyType,ValType> &leaf) -> page_id_t;
        auto SplitInternal(BPlusInternalPage<KeyType> &internal_page);
        auto InsertInfoParent(page_id_t old_page,const KeyType &middle_key,page_id_t new_page) -> void;

        std::shared_ptr<BufferManager> buffer_manager_;
        page_id_t root_page_id_{INVALID_PAGE_ID};
    };

}
