//
// Created by 杨欢 on 2026/3/15.
//
#include "index/index_page.h"

#include <stdexcept>

#include "common/util/page_id_util.h"
using namespace YourSQL;

auto IndexPage::Init(IndexPageType type, page_id_t page_id, page_id_t parent_id, uint16_t max_size) -> void {
    if (page_ == nullptr) {
        throw std::runtime_error("IndexPage::Init page_ is nullptr.");
    }

    size_t offset = 0;
    memcpy(page_->data_+offset,&type,sizeof(IndexPageType));
    offset += sizeof(IndexPageType);
    uint16_t size = 0;
    memcpy(page_->data_+offset,&size,sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(page_->data_+offset,&max_size,sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(page_->data_+offset,&page_id,sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(page_->data_+offset,&parent_id,sizeof(page_id_t));

    header_.type_ = type;
    header_.page_id_ = page_id;
    header_.parent_page_id_ = parent_id;
    header_.max_size_ = max_size;
    header_.size_ = 0;

    page_->is_dirty_ = true;
}

auto IndexPage::SetParentPageId(page_id_t parent_id) -> void {
    memcpy(page_->data_+INDEX_HEADER_PARENT_PAGE_ID_OFFSET,&parent_id,sizeof(page_id_t));
    page_->is_dirty_ = true;
}

auto IndexPage::SetSize(uint16_t size) -> void {
    memcpy(page_->data_+INDEX_HEADER_SIZE_OFFSET,&size,sizeof(uint16_t));
    page_->is_dirty_ = true;
}

auto IndexPage::IsLeafPage() const -> bool {
    return header_.type_ == IndexPageType::LEAF;
}

auto IndexPage::IncreaseSize(int delta) -> void {
    uint16_t size = GetSize();
    SetSize(size + delta);
    page_->is_dirty_ = true;
}

auto IndexPage::Header() -> IndexPageHeader * {
    return &header_;
}

auto IndexPage::GetSize() const -> uint16_t {
    return header_.size_;
}

auto IndexPage::GetParentPageId() const -> page_id_t {
    return header_.parent_page_id_;
}

auto IndexPage::GetPageId() const -> page_id_t {
    return header_.page_id_;
}

auto IndexPage::GetMinSize() const -> uint16_t {
    return 0;
}

auto IndexPage::GetMaxSize() const -> uint16_t {
    return header_.max_size_;
}
