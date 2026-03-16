//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_leaf_page.h"

#include "common/util/page_id_util.h"

using namespace YourSQL;

template<class KeyType, class ValType>
BPlusLeafPage<KeyType, ValType>::BPlusLeafPage(Page *page) : IndexPage(page) {
    uint16_t size = GetSize();
    if (size <= 0) return;

    size_t offset = INDEX_COMMON_HEADER_SIZE;
    for (size_t i = 0; i < size; i++) {
        size_t len = 0;
        memcpy(&len, page_->data_ + offset, sizeof(len));
        offset += sizeof(size_t);
        KeyType key;
        memcpy(&key, page_->data_ + offset, len);
        offset += len;

        size_t vlen = 0;
        memcpy(&vlen, page_->data_ + offset, sizeof(size_t));
        offset += sizeof(size_t);
        ValType val;
        memcpy(&val, page_->data_ + offset, sizeof(vlen));
        offset += vlen;
        array_.push_back(MappingType(key, val));
    }
}


template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::ValueAt(int index) const -> const ValType & {
    return array_[index];
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::SetNextPageId(page_id_t next_page_id) const -> void {
    memcpy(page_->data_ + INDEX_COMMON_HEADER_SIZE, &next_page_id, sizeof(next_page_id));
    page_->is_dirty_ = true;
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Remove(const KeyType &key, const ValType &value) -> int {
    if (array_.empty()) return 0;
    char *data_start = page_->data_ + sizeof(IndexPageHeader) + sizeof(LeafExtraHeader);
    char *current = data_start;
    for (int i = 0; i < GetSize(); i++) {
        size_t key_len;
        std::memcpy(&key_len, current, sizeof(size_t));
        current += sizeof(size_t);

        KeyType current_key;
        std::memcpy(&current_key, current, key_len);
        current += key_len;

        size_t val_len;
        std::memcpy(&val_len, current, sizeof(size_t));
        current += sizeof(size_t);

        ValType current_val;
        std::memcpy(&current_val, current, val_len);
        current += val_len;

        if (current_key == key && current_val == value) {
            size_t total_size = sizeof(size_t) + key_len + sizeof(size_t) + val_len;
            char *elem_start = current - total_size;

            char *remaining_start = current;
            char *data_end = data_start + GetUsedBytes();

            size_t remaining_bytes = data_end - remaining_start;
            std::memmove(elem_start, remaining_start, remaining_bytes);
            IncreaseSize(-1);
            page_->is_dirty_ = true;
            return 1;
        }
    }
    return 0;
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::GetUsedBytes() const -> size_t {
    char *data_start = page_->data_ + sizeof(IndexPageHeader) + sizeof(LeafExtraHeader);
    char *current = data_start;

    for (int i = 0; i < GetSize(); i++) {
        size_t key_len;
        std::memcpy(&key_len, current, sizeof(size_t));
        current += sizeof(size_t) + key_len;

        size_t val_len;
        std::memcpy(&val_len, current, sizeof(size_t));
        current += sizeof(size_t) + val_len;
    }
    return current - data_start;
}


template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::LowerBound(const KeyType &key) const -> int {
    auto it = std::lower_bound(array_.begin(), array_.end(), key, [&](const auto &item, const KeyType &k) {
        return item.first < k;
    });
    return std::distance(array_.begin(), it);
}


template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Lookup(const KeyType &key, std::vector<ValType> &result) const -> bool {
    if (array_.empty()) return false;

    int start = LowerBound(key);
    if (start >= GetSize() || KeyAt(start) != key) return false;

    for (size_t i = start; i < GetSize() && KeyAt(i) == key; i++) {
        result.push_back(array_[i].second);
    }
    return true;
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::KeyAt(int index) const -> const KeyType & {
    return array_[index].first;
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Insert(const KeyType &key, const ValType &value) -> bool {
    if (GetSize() >= GetMaxSize()) return false;

    size_t insert_pos = LowerBound(key);

    array_.insert(array_.begin() + insert_pos, MappingType(key,value));

    IncreaseSize(1);
    SerializeToPage();
    return true;
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::SerializeToPage() -> void {
    size_t offset = sizeof(IndexPageHeader) + sizeof(LeafExtraHeader);
    for (auto array : array_) {
        size_t key_len = sizeof(KeyType);
        size_t val_len = sizeof(ValType);

        memcpy(page_->data_ + offset, &key_len, sizeof(size_t));
        offset += sizeof(size_t);
        memcpy(page_->data_ + offset, &array.first, key_len);
        offset += key_len;

        memcpy(page_->data_ + offset, &val_len, sizeof(size_t));
        offset += sizeof(size_t);
        memcpy(page_->data_ + offset, &array.second, val_len);
        offset += val_len;
    }

    page_->is_dirty_ = true;
}



template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Init(page_id_t page_id, page_id_t parent_id, uint16_t max_size) -> void {
    IndexPage::Init(IndexPageType::LEAF, page_id, parent_id, max_size);
    extra_header_.next_page_id_ = GetNextPageId();
}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::GetNextPageId() const -> page_id_t {
    page_id_t next_page_id = 0;
    memcpy(&next_page_id, page_->data_ + INDEX_COMMON_HEADER_SIZE, sizeof(page_id_t));
    return next_page_id;
}
