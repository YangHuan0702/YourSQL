//
// Created by 杨欢 on 2026/3/15.
//
#include "index/index_page.h"
using namespace YourSQL;

auto IndexPage::Init(IndexPageType type, page_id_t page_id, page_id_t parent_id, uint16_t max_size) -> void {

}

auto IndexPage::SetParentPageId(page_id_t parent_id) -> void {

}

auto IndexPage::SetSize(int size) -> void {

}

auto IndexPage::IsLeafPage() const -> bool {

}

auto IndexPage::IncreaseSize(int delta) -> void {

}

auto IndexPage::Header() -> IndexPageHeader * {

}

auto IndexPage::GetSize() const -> int {

}

auto IndexPage::GetParentPageId() const -> page_id_t {

}

auto IndexPage::GetPageId() const -> page_id_t {

}

auto IndexPage::GetMinSize() const -> int {

}

auto IndexPage::GetMaxSize() const -> int {

}
