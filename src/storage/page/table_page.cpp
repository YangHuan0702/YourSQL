//
// Created by huan.yang on 2026-03-03.
//
#include "storage/page/table_page.h"

using namespace YourSQL;


TablePage::TablePage(Page *page) : page_(page),free_size(PAGE_SIZE - HEADER_SIZE) {
    char *data = page->data_;
    memcpy(&header_.version, data, NUM_ROWS_OFFSET);
    memcpy(&header_.num_rows,data+NUM_ROWS_OFFSET,sizeof(uint32_t));
    memcpy(&header_.page_id,data+NUM_ROWS_OFFSET + sizeof(uint32_t),sizeof(uint64_t));
    memcpy(&header_.next_page_id,data+NUM_ROWS_OFFSET + sizeof(uint32_t) + sizeof(uint64_t),sizeof(uint64_t));

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
    std::lock_guard lock(mutex_);
    size_t offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    uint16_t slot_offset = 0;
    uint16_t size = 0;
    char deleted = 0;
    memcpy(&slot_offset,page_->data_+offset,sizeof(uint16_t));
    memcpy(&size,page_->data_+offset+sizeof(uint16_t),sizeof(uint16_t));
    memcpy(&deleted,page_->data_+offset+sizeof(uint16_t) * 2 + 1,sizeof(char));

    if (!deleted) {
        auto target = new char[size];
        memcpy(target,page_->data_+slot_offset,size);
        tuple->data_ = target;
    }
}


auto TablePage::InsertTuple(const Tuple &tuple,RID *rid) -> bool {
    std::lock_guard lock(mutex_);
    if (tuple.tuple_size_ + SLOT_SIZE > free_size) {
        return false;
    }

    int cur_slot_point = PAGE_SIZE - (SLOT_SIZE * header_.num_rows);
    uint16_t data_point = cur_slot_point - free_size;
    memcpy(page_->data_ + data_point, tuple.data_, tuple.tuple_size_);

    int new_slot_offset = cur_slot_point - SLOT_SIZE;
    memcpy(page_->data_ + new_slot_offset, &data_point, sizeof(uint16_t));
    memcpy(page_->data_ + new_slot_offset + sizeof(uint16_t), &tuple.tuple_size_, sizeof(uint16_t));
    char del = 0;
    memcpy(page_->data_ + new_slot_offset + sizeof(uint16_t) * 2 + 1, &del, sizeof(char));

    free_size -= SLOT_SIZE + tuple.tuple_size_;
    header_.num_rows += 1;
    rid->row_id_ = header_.num_rows;
    page_->is_dirty_ = true;
    return true;
}




auto TablePage::updateTuple(const Tuple &tuple, const RID &rid) -> void {
    std::lock_guard lock(mutex_);
    int slot_offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;
    Slot slot{};
    memcpy(&slot.offset, page_->data_ + slot_offset, sizeof(uint16_t));
    memcpy(&slot.size, page_->data_ + slot_offset + sizeof(uint16_t), sizeof(uint16_t));
    memcpy(&slot.deleted, page_->data_ + slot_offset + sizeof(uint16_t)*2 + 1, sizeof(char));

    if (slot.deleted) {
        return;
    }

    size_t prev_size = 0;
    for (size_t i = 1; i < rid.row_id_; ++i) {
        size_t offset = PAGE_SIZE - i * SLOT_SIZE + sizeof(uint16_t);
        uint16_t size = 0;
        memcpy(&size, page_->data_+offset, sizeof(uint16_t));
        prev_size += size;
    }

    if (slot.size <= tuple.tuple_size_) {
        memcpy(page_->data_ + slot.offset, tuple.data_, tuple.tuple_size_);
        memmove(page_->data_ + slot.offset + tuple.tuple_size_, page_->data_ + slot.offset + slot.size, prev_size);
    } else {
        size_t diff_size = tuple.tuple_size_ - slot.size;
        if (free_size < diff_size) {
            // (TODO)删除当前页的数据并新增页去构建
        }
        memcpy(page_->data_ + slot_offset + sizeof(uint16_t), &tuple.tuple_size_, sizeof(uint16_t));
        memmove(page_->data_+slot.offset + slot.size, page_->data_+slot.offset + slot.size+diff_size,prev_size);
        memcpy(page_->data_ + slot.offset, &tuple.data_, tuple.tuple_size_);
    }
}


auto TablePage::GetCount() -> uint32_t {
    return header_.num_rows;
}


auto TablePage::DeleteTuple(const RID &rid) -> void {
    std::lock_guard lock(mutex_);

    int offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;
    char del = 0;
    memcpy(&del, page_->data_ + offset + sizeof(uint16_t) * 2 + 1, sizeof(char));

    if (!del) {
        del = 1;
        memcpy(page_->data_ + offset + sizeof(uint16_t) * 2 + 1,&del,sizeof(char));
    }
}
