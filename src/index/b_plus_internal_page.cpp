//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_internal_page.h"
using namespace YourSQL;

template<class KeyType>
auto BPlusInternalPage<KeyType>::SetValueAt(int index, page_id_t child) -> void {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::SetKeyAt(int index, const KeyType &key) -> void {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::Remove(int index) -> void {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::LookupChild(const KeyType &key) const -> page_id_t {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::InsertAfter(page_id_t old_child, const KeyType &middle_key, page_id_t new_child) -> void {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::ValueAt(int index) const -> page_id_t {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::KeyAt(int index) const -> const KeyType & {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::Init(page_id_t page_id, page_id_t parent_id, int max_size) -> void {

}

template<class KeyType>
auto BPlusInternalPage<KeyType>::Array() -> MappingType * {

}
