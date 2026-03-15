//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_index_iterator.h"
using namespace YourSQL;

template<class KeyType, class ValType>
BPlusIndexIterator<KeyType, ValType>::BPlusIndexIterator(std::shared_ptr<BufferManager> buffer_manager, page_id_t leaf_page_id, int index) {

}

template<class KeyType, class ValType>
BPlusIndexIterator<KeyType, ValType>::~BPlusIndexIterator() {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::ReleaseLeaf() -> void {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator==(const BPlusIndexIterator &other) const -> bool {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator++() -> BPlusIndexIterator & {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::operator*() const -> MappingType {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::LoadLeaf(page_id_t leaf_page_id) -> void {

}

template<class KeyType, class ValType>
auto BPlusIndexIterator<KeyType, ValType>::IsEnd() const -> bool {

}
