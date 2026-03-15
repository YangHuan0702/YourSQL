//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_leaf_page.h"

using namespace YourSQL;

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::ValueAt(int index) const -> const ValType & {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::SetNextPageId(page_id_t next_page_id) -> void {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Remove(const KeyType &key, const ValType &value) -> int {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::LowerBound(const KeyType &key) const -> int {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Lookup(const KeyType &key, std::vector<ValType> &result) const -> bool {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::KeyAt(int index) const -> const KeyType & {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Insert(const KeyType &key, const ValType &value) -> bool {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Init(page_id_t page_id, page_id_t parent_id, int max_size) -> void {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::GetNextPageId() const -> page_id_t {

}

template<class KeyType, class ValType>
auto BPlusLeafPage<KeyType, ValType>::Array() -> MappingType * {

}
