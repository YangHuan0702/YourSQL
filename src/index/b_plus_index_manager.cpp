//
// Created by 杨欢 on 2026/3/15.
//
#include "index/b_plus_index_manager.h"
#include "glog/logging.h"

using namespace YourSQL;

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Delete(const KeyType &key) -> bool {
    if (root_page_id_ == INVALID_PAGE_ID) {
        LOG(ERROR) << "BPlusIndexManager::Delete: root page not found,bec root_page_id == INVALID_PAGE_ID";
        return false;
    }

    page_id_t leaf_id = FindLeafPage(key, false);
    Page *page = buffer_manager_->FetchPage(leaf_id);
    auto leaf_page = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page->data_);

    int r = leaf_page->Remove(key);

    if (!r) {
        buffer_manager_->Release(leaf_page);
        return false;
    }

    if (leaf_id != root_page_id_ && leaf_page->GetSize() < leaf_page->GetMinSize()) {
        HandleUnderflow(leaf_page);
    } else if (leaf_id == root_page_id_ && leaf_page->GetSize() == 0) {
        root_page_id_ = INVALID_PAGE_ID;
    }
    buffer_manager_->Release(leaf_page);
    return true;
}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::HandleUnderflow(BPlusLeafPage<KeyType, ValType> *leaf_node) -> void {
    // 如果是根节点
    if (leaf_node->GetPageId() == root_page_id_) {
        AdjustRoot(leaf_node);
        return;
    }

    Page *parent_page = buffer_manager_->FetchPage(leaf_node->GetParentPageId());
    auto parent = reinterpret_cast<BPlusInternalPage<KeyType> *>(parent_page->data_);

    int index_in_parent = -1;
    for (int i = 0; i < parent->GetSize(); i++) {
        if (parent->ValueAt(i) == leaf_node->GetPageId()) {
            index_in_parent = i;
            break;
        }
    }

    if (index_in_parent > 0) {
        Page *left_page = buffer_manager_->FetchPage(parent->ValueAt(index_in_parent - 1));
        auto left_sibling = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(left_page->data_);

        if (left_sibling->GetSize() > left_sibling->GetMinSize()) {
            Redistribute(left_sibling, leaf_node, parent, true);

            buffer_manager_->Release(left_page->id_);
            buffer_manager_->Release(parent_page->id_);
            return;
        }

        Coalesce(left_sibling, leaf_node, parent, index_in_parent);

        buffer_manager_->Release(left_page->id_);
    } else {
        Page *right_page = buffer_manager_->FetchPage(parent->ValueAt(index_in_parent + 1));
        auto right_sibling = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(right_page->data_);

        if (right_sibling->GetSize() > right_sibling->GetMinSize()) {
            Redistribute(right_sibling, leaf_node, parent, false);

            buffer_manager_->Release(right_page->id_);
            buffer_manager_->Release(parent_page->id_);
            return;
        }

        Coalesce(leaf_node, right_sibling, parent, index_in_parent + 1);
        buffer_manager_->Release(right_page->id_);
    }

    buffer_manager_->Release(parent_page->id_);
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Insert(const KeyType &key, const ValType &val) -> bool {
    if (root_page_id_ == INVALID_PAGE_ID) {
        Page *root_page = buffer_manager_->NewPage();
        if (root_page == nullptr) {
            LOG(ERROR) << "BPlusIndexManager::Insert: create root page is null!";
            return false;
        }
        root_page->Reset();
        root_page_id_ = root_page->id_;
        auto root_leaf = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(root_page->data_);
        root_leaf->Init(root_page->id_,INVALID_PAGE_ID, root_leaf->GetMaxSize());
        root_leaf->Insert(key, val);

        buffer_manager_->Release(root_page->id_);

        // TODO：修改root Page元数据
        return true;
    }

    page_id_t leaf_page_id = FindLeafPage(key, false);

    Page *page = buffer_manager_->FetchPage(leaf_page_id);
    if (page == nullptr)
        throw std::runtime_error(
            "BPlusIndexManager::Insert don`t find page for buffer_manager : " + std::to_string(leaf_page_id));

    auto leaf_page = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page->data_);
    bool r = leaf_page->Insert(key, val);
    if (!r) {
        buffer_manager_->Release(leaf_page_id);
        throw std::runtime_error("BPlusIndexManager::Insert due key.");
    }

    // 分裂
    if (leaf_page->GetSize() >= leaf_page->GetMaxSize()) {
        page_id_t new_leaf_page_id = SplitLeaf(*leaf_page);

        Page *new_page = buffer_manager_->FetchPage(new_leaf_page_id);
        if (new_page == nullptr)
            throw std::runtime_error(
                "BPlusIndexManager::Insert split of the new page fetch error was buffer_manager: " + std::to_string(
                    new_leaf_page_id));
        auto new_leaf_node = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(new_page->data_);

        KeyType middle_key = new_leaf_node->KeyAt(0);
        buffer_manager_->Release(new_leaf_page_id);

        InsertInfoParent(leaf_page_id, middle_key, new_leaf_page_id);
    }

    buffer_manager_->Release(leaf_page_id);
    return true;
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Get(const KeyType &key, std::vector<ValType> &re) -> bool {
    if (root_page_id_ == INVALID_PAGE_ID) {
        LOG(INFO) << "BPlusIndexManager::Get Root page id is INVALID_PAGE_ID";
        return false;
    }
    auto page_id = FindLeafPage(key, false);
    Page *page = buffer_manager_->FetchPage(page_id);
    auto leafPage = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page->data_);
    bool r = leafPage->Lookup(key, re);
    buffer_manager_->Release(page_id);
    return r;
}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::InsertInfoParent(page_id_t old_page, const KeyType &middle_key,
                                                           page_id_t new_page) -> void {
    if (old_page == root_page_id_) {
        Page *page = buffer_manager_->NewPage();
        page_id_t new_root_id = page->id_;
        auto *new_root = reinterpret_cast<BPlusInternalPage<KeyType> *>(page->data_);

        new_root->Init(new_root_id, INVALID_PAGE_ID, new_root->GetMaxSize());
        new_root->SetValueAt(0, old_page);
        new_root->SetKeyAt(1, middle_key);
        new_root->SetValueAt(1, new_page);
        new_root->SetSize(2);

        root_page_id_ = new_root_id;

        UpdateParentId(old_page, new_root_id);
        UpdateParentId(new_page, new_root_id);

        buffer_manager_->Release(new_root_id);
        return;
    }

    Page *page = buffer_manager_->FetchPage(GetParentId(old_page));
    auto parent = reinterpret_cast<BPlusInternalPage<KeyType> *>(page->data_);

    parent->InsertAfter(old_page, middle_key, new_page);

    if (parent->GetSize() >= parent->GetMaxSize()) {
        auto [up_key,up_new_page_id] = SplitInternal(*parent);
        InsertInfoParent(parent->GetPageId(), up_key, up_new_page_id);
    }
    buffer_manager_->Release(parent->GetPageId());
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::SplitLeaf(BPlusLeafPage<KeyType, ValType> &leaf) -> page_id_t {
    Page *page = buffer_manager_->NewPage();
    auto new_node = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(page->data_);

    new_node->Init(page->id_, leaf.GetParentPageId(), leaf.GetMaxSize());

    int split_at = leaf.GetSize() / 2;
    for (int i = split_at; i < leaf.GetSize(); i++) {
        new_node->Insert(leaf.KeyAt(i), leaf.ValAt(i));
    }

    leaf.SetSize(split_at);

    new_node->SetNextPageId(leaf.GetNextPageId());
    leaf.SetNextPageId(page->id_);

    buffer_manager_->Release(page->id_);
    return page->id_;
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::SplitInternal(BPlusInternalPage<KeyType> &internal_page) {
    Page *page = buffer_manager_->NewPage();
    auto new_node = reinterpret_cast<BPlusInternalPage<KeyType> *>(page->data_);
    new_node->Init(page->id_, internal_page.GetParentPageId(), internal_page.GetMaxSize());

    int size = internal_page.GetSize();
    int split_at = size / 2;

    KeyType middle_key = internal_page.KeyAt(split_at);

    for (int i = split_at; i < size; i++) {
        new_node->SetKeyAt(i - split_at, internal_page.KeyAt(i));
        new_node->SetValueAt(i - split_at, internal_page.ValAt(i));

        UpdateParentId(internal_page.ValueAt(i), page->id_);
    }

    new_node->SetSize(size - split_at);
    internal_page.SetSize(split_at);

    buffer_manager_->Release(page->id_);
    return std::make_pair(middle_key, page->id_);
}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::FindLeafPage(const KeyType &key, bool leaf_most) -> page_id_t {
    if (root_page_id_ == INVALID_PAGE_ID) {
        throw std::runtime_error("BPlusInsertManager::FindLeafPage root_page_id is INVALID_PAGE_ID");
    }

    page_id_t cur_page_id = root_page_id_;
    Page *page = buffer_manager_->FetchPage(cur_page_id);
    auto node = reinterpret_cast<IndexPage *>(page->data_);

    while (!node->IsLeafPage()) {
        auto internal_page = reinterpret_cast<BPlusInternalPage<KeyType> *>(node);
        page_id_t next_id = leaf_most ? internal_page->ValueAt(0) : internal_page->LookupChild(key);

        buffer_manager_->Release(cur_page_id);

        cur_page_id = next_id;
        page = buffer_manager_->FetchPage(cur_page_id);
        node = reinterpret_cast<IndexPage *>(page->data_);
    }

    buffer_manager_->Release(cur_page_id);
    return cur_page_id;
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::GetParentId(page_id_t page_id) const -> page_id_t {
    Page *page = buffer_manager_->FetchPage(page_id);
    auto index_page = reinterpret_cast<IndexPage *>(page->data_);
    auto r = index_page->GetParentPageId();
    buffer_manager_->Release(page_id);
    return r;
}

template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType,
    ValType>::UpdateParentId(page_id_t page_id, page_id_t new_parent_page_id) const -> void {
    Page *page = buffer_manager_->FetchPage(page_id);
    auto index_page = reinterpret_cast<IndexPage *>(page->data_);
    index_page->SetParentPageId(new_parent_page_id);
    buffer_manager_->Release(page_id);
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::AdjustRoot(IndexPage *old_root) -> void {
    if (old_root->IsLeafPage() && old_root->GetSize() == 0) {
        root_page_id_ = INVALID_PAGE_ID;
        buffer_manager_->Release(old_root->GetPageId());
        return;
    }


    if (!old_root->IsLeafPage() && old_root->GetSize() == 1) {
        auto internal_root = reinterpret_cast<BPlusInternalPage<KeyType> *>(old_root);
        page_id_t new_root_id = internal_root->ValueAt(0);
        root_page_id_ = new_root_id;

        Page *new_root_page = buffer_manager_->FetchPage(new_root_id);
        auto *new_root_node = reinterpret_cast<IndexPage *>(new_root_page->data_);
        new_root_node->SetParentPageId(INVALID_PAGE_ID);

        buffer_manager_->Release(new_root_id);
        buffer_manager_->Release(old_root->GetPageId());
    }
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Redistribute(IndexPage *sibling,
                                                       IndexPage *node,
                                                       BPlusInternalPage<KeyType> *parent, bool is_left_sibling) -> void {
    if (node->IsLeafPage()) {
        auto leaf_node = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(node);
        auto sibling_leaf = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(sibling);

        if (is_left_sibling) {
            int last_idx = sibling_leaf->GetSize() - 1;
            leaf_node->Insert(sibling_leaf->KeyAt(last_idx), sibling_leaf->ValueAt(last_idx));
            sibling_leaf->Remove(sibling_leaf->KeyAt(last_idx));

            for (int i = 0; i < parent->GetSize(); i++) {
                if (parent->ValueAt(i) == leaf_node->GetPageId()) {
                    parent->SetKeyAt(i, leaf_node->KeyAt(0));
                    break;
                }
            }
        } else {
            leaf_node->Insert(sibling_leaf->KeyAt(0), sibling_leaf->ValueAt(0));
            sibling_leaf->Remove(sibling_leaf->KeyAt(0));

            for (int i = 0; i < parent->GetSize(); i++) {
                if (parent->ValueAt(i) == sibling_leaf->GetPageId()) {
                    parent->SetKeyAt(i, sibling_leaf->KeyAt(0));
                    break;
                }
            }
        }
    } else {
        auto internal_node = reinterpret_cast<BPlusInternalPage<KeyType> *>(node);
        auto sibling_internal = reinterpret_cast<BPlusInternalPage<KeyType> *>(sibling);

        if (is_left_sibling) {
            int last_idx = sibling_internal->GetSize() - 1;
            KeyType key = sibling_internal->KeyAt(last_idx);
            page_id_t child = sibling_internal->ValueAt(last_idx);

            internal_node->InsertAfter(internal_node->ValueAt(0), key, child);
            sibling_internal->Remove(last_idx);

            UpdateParentId(child, internal_node->GetPageId());
        } else {
            KeyType key = sibling_internal->KeyAt(0);
            page_id_t child = sibling_internal->ValueAt(0);

            int last_idx = internal_node->GetSize() - 1;
            internal_node->InsertAfter(internal_node->ValueAt(last_idx), key, child);
            sibling_internal->Remove(0);

            UpdateParentId(child, internal_node->GetPageId());
        }
    }
}


template<class KeyType, class ValType>
auto BPlusIndexManager<KeyType, ValType>::Coalesce(IndexPage *sibling,
                                                   IndexPage *node,
                                                   BPlusInternalPage<KeyType> *parent, int index) {
    if (node->IsLeafPage()) {
        auto leaf_node = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(node);
        auto sibling_leaf = reinterpret_cast<BPlusLeafPage<KeyType, ValType> *>(sibling);

        for (int i = 0; i < leaf_node->GetSize(); i++) {
            sibling_leaf->Insert(leaf_node->KeyAt(i), leaf_node->ValueAt(i));
        }

        sibling_leaf->SetNextPageId(leaf_node->GetNextPageId());
    } else {
        auto internal_node = reinterpret_cast<BPlusInternalPage<KeyType> *>(node);
        auto sibling_internal = reinterpret_cast<BPlusInternalPage<KeyType> *>(sibling);

        for (int i = 0; i < internal_node->GetSize(); i++) {
            int last_idx = sibling_internal->GetSize() - 1;
            sibling_internal->InsertAfter(sibling_internal->ValueAt(last_idx),
                                         internal_node->KeyAt(i),
                                         internal_node->ValueAt(i));
            UpdateParentId(internal_node->ValueAt(i), sibling_internal->GetPageId());
        }
    }

    parent->Remove(index);

    if (parent->GetPageId() == root_page_id_ && parent->GetSize() == 1) {
        AdjustRoot(parent);
    }
}
