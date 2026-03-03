//
// Created by huan.yang on 2026-03-03.
//
#include "storage/page/table_page.h"

using namespace YourSQL;


TablePage::TablePage(Page *page) : page_(page),free_size(PAGE_SIZE - HEADER_SIZE) {
    char *data = page->data_;
    memcpy(&header_.version, data, sizeof(uint16_t));
    memcpy(&header_.num_rows,data+NUM_ROWS_OFFSET,sizeof(uint32_t));

    uint32_t slot_count_size = SLOT_SIZE * header_.num_rows;
    uint32_t tuple_count_size = 0;
    for (uint32_t i = 1; i <= header_.num_rows; ++i) {
        size_t offset = PAGE_SIZE - i * SLOT_SIZE + sizeof(uint16_t);
        uint16_t size = 0;
        memcpy(&size, data+offset, sizeof(uint16_t));
        tuple_count_size += size;
    }

    free_size -= slot_count_size + tuple_count_size;
}


auto TablePage::GetTuple(const RID &rid, Tuple *tuple) {
    size_t offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    uint16_t slot_offset = 0;
    uint16_t size = 0;
    bool deleted = false;
    memcpy(&slot_offset,page_->data_+offset,sizeof(uint16_t));
    memcpy(&size,page_->data_+offset+sizeof(uint16_t),sizeof(uint16_t));
    memcpy(&deleted,page_->data_+offset+sizeof(uint16_t) * 2,sizeof(bool));

    if (!deleted) {
        auto target = new char[size];
        memcpy(target,page_->data_+slot_offset,size);
        tuple->data_ = target;
    }
}


auto TablePage::InsertTuple(const Tuple &tuple, const RID &rid) -> void {

}


auto TablePage::updateTuple(const Tuple &tuple, const RID &rid) -> void {

}


auto TablePage::GetCount() -> int {
    return header_.num_rows;
}


auto TablePage::DeleteTuple(const RID &rid) -> void {

}
