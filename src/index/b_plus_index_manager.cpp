//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_index_manager.h"
using namespace YourSQL;

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Delete(const KeyType &key) -> bool {

}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Insert(const KeyType &key, const ValType &val) -> bool {

}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Get(const KeyType &key, std::vector<ValType> &re) -> bool {

}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::InsertInfoParent(page_id_t old_page, const KeyType &middle_key, page_id_t new_page) -> void {

}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::SplitLeaf(BPlusLeafPage<KeyType, ValType> &leaf) -> page_id_t {

}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::SplitInternal(BPlusInternalPage<KeyType> &internal_page) {

}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::FindLeafPage(const KeyType &key, bool leaf_most) -> page_id_t {

}
