//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_internal_page.h"

#include <algorithm>
#include <stdexcept>
using namespace YourSQL;


template<class KeyType>
auto BPlusInternalPage<KeyType>::SetValueAt(int index, page_id_t child) -> void {
    array_[index].second = child;
    size_t key_len = sizeof(KeyType);
    memcpy(page_->data_ + INDEX_COMMON_HEADER_SIZE + (index * ITEM_SIZE) + sizeof(size_t) + key_len, &child, sizeof(page_id_t));
    page_->is_dirty_ = true;
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::SetKeyAt(int index, const KeyType &key) -> void {
    array_[index].first = key;
    memcpy(page_->data_ + INDEX_COMMON_HEADER_SIZE + (index * ITEM_SIZE) + sizeof(size_t), &key, sizeof(KeyType));
    page_->is_dirty_ = true;
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::Remove(int index) -> void {
    if (index < 0 || index >= array_.size()) {
        throw std::runtime_error("BPlusInternalPage::Remove: Invalid index");
    }
    array_.erase(array_.begin() + index);
    IncreaseSize(-1);
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::LookupChild(const KeyType &key) const -> page_id_t {
    auto it = std::lower_bound(array_.begin(),array_.end(),key,[&](const auto &item, const KeyType &k)->bool {
        return item.first < k;
    });
    return array_[it - array_.begin()].second;
}


template<class KeyType>
auto BPlusInternalPage<KeyType>::InsertAfter(page_id_t old_child, const KeyType &middle_key, page_id_t new_child) -> void {
    auto it = std::find_if(array_.begin(),array_.end(),[old_child](const auto &item) {
       return item.second == old_child;
    });

    if (it != array_.end()) {
        array_.insert(it+1,{middle_key,new_child});
        IncreaseSize(1);
    }
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::ValueAt(int index) const -> page_id_t {
    return array_[index].second;
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::KeyAt(int index) const -> const KeyType & {
    return array_[index].first;
}

template<class KeyType>
auto BPlusInternalPage<KeyType>::Init(page_id_t page_id, page_id_t parent_id, int max_size) -> void {
    IndexPage::Init(IndexPageType::INTERVAL,page_id,parent_id,max_size);
}

