//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_index_iterator.h"
#include "index/b_plus_leaf_page.h"

using namespace YourSQL;

template<class KeyType, class ValType>
BPlusIndexIterator<KeyType, ValType>::BPlusIndexIterator(std::shared_ptr<BufferManager> buffer_manager, page_id_t leaf_page_id, int index)
    : buffer_manager_(buffer_manager), leaf_page_id_(leaf_page_id), index_(index), is_end_(false) {
    LoadLeaf(leaf_page_id);
}

template<class KeyType, class ValType>
BPlusIndexIterator<KeyType, ValType>::~BPlusIndexIterator() {
    ReleaseLeaf();
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::ReleaseLeaf() -> void {
    if (page_ != nullptr && buffer_manager_ != nullptr) {
        buffer_manager_->Release(page_);
        page_ = nullptr;
    }
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator==(const BPlusIndexIterator &other) const -> bool {
    return leaf_page_id_ == other.leaf_page_id_ && index_ == other.index_ && is_end_ == other.is_end_;
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator++() -> BPlusIndexIterator & {
    if (is_end_ || page_ == nullptr) {
        return *this;
    }

    auto leaf_page = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page_->data_);
    index_++;

    if (index_ >= leaf_page->GetSize()) {
        page_id_t next_page_id = leaf_page->GetNextPageId();
        if (next_page_id == INVALID_PAGE_ID) {
            is_end_ = true;
            ReleaseLeaf();
        } else {
            LoadLeaf(next_page_id);
            index_ = 0;
        }
    }

    return *this;
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator*() const -> MappingType {
    if (is_end_ || page_ == nullptr) {
        throw std::runtime_error("BPlusIndexIterator: dereferencing end iterator");
    }

    auto leaf_page = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page_->data_);
    return {leaf_page->KeyAt(index_), leaf_page->ValueAt(index_)};
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::LoadLeaf(page_id_t leaf_page_id) -> void {
    if (leaf_page_id == INVALID_PAGE_ID) {
        is_end_ = true;
        return;
    }

    ReleaseLeaf();
    page_ = buffer_manager_->FetchPage(leaf_page_id);
    leaf_page_id_ = leaf_page_id;

    if (page_ == nullptr) {
        is_end_ = true;
    }
}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::IsEnd() const -> bool {
    return is_end_;
}
