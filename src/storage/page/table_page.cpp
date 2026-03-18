//
// Created by huan.yang on 2026-03-03.
//
#include "storage/page/table_page.h"

using namespace YourSQL;


TablePage::TablePage(std::shared_ptr<MetaPage> meta_page, entry_id table_id, Page *page,
                     bool read) : meta_page_(std::move(meta_page)), table_id_(table_id), page_(page),
                                  free_size(PAGE_SIZE - HEADER_SIZE) {
    char *data = page->data_;
    if (read) {
        memcpy(&header_.version, data, NUM_ROWS_OFFSET);
        memcpy(&header_.num_rows, data + NUM_ROWS_OFFSET, sizeof(uint32_t));
        memcpy(&header_.page_id, data + NUM_ROWS_OFFSET + sizeof(uint32_t), sizeof(uint64_t));
        memcpy(&header_.next_page_id, data + NUM_ROWS_OFFSET + sizeof(uint32_t) + sizeof(uint64_t), sizeof(uint64_t));

        uint32_t slot_count_size = SLOT_SIZE * header_.num_rows;
        uint32_t tuple_count_size = 0;
        for (uint32_t i = 1; i <= header_.num_rows; ++i) {
            size_t offset = PAGE_SIZE - i * SLOT_SIZE + sizeof(uint16_t);
            uint16_t size = 0;
            memcpy(&size, data + offset, sizeof(uint16_t));
            tuple_count_size += size;
        }
        free_size -= slot_count_size + tuple_count_size;
    } else {
        // init
        header_.version = 0;
        header_.num_rows = 0;
        header_.page_id = page->id_;
        header_.next_page_id = 0;

        size_t offset = 0;
        memcpy(data + offset, &header_.version, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(data + offset, &header_.num_rows, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        memcpy(data + offset, &header_.page_id, sizeof(page_id_t));
        offset += sizeof(page_id_t);
        memcpy(data + offset, &header_.next_page_id, sizeof(page_id_t));

        page_->is_dirty_ = true;
    }
}


auto TablePage::GetTuple(const RID &rid, Tuple *tuple, const Transaction &transaction) -> void {
    std::lock_guard lock(mutex_);
    size_t offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    uint16_t slot_offset = 0;
    uint16_t size = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));
    memcpy(&size, page_->data_ + offset + sizeof(uint16_t), sizeof(uint16_t));

    tx_id_t xmin = 0;
    memcpy(&xmin, page_->data_ + offset + XMIN_OFFSET, sizeof(tx_id_t));

    tx_id_t xmax = 0;
    memcpy(&xmax, page_->data_ + offset + XMAX_OFFSET, sizeof(tx_id_t));

    int16_t info_mask = 0;
    memcpy(&info_mask, page_->data_ + offset + INFO_MASK_OFFSET, sizeof(int16_t));

    if (!(info_mask & HEAP_XMAX_INVALID) && !(info_mask & HEAP_UPDATED) && transaction.GetSnapShot().IsVisible(xmin)) {
        auto target = new char[size];
        memcpy(target, page_->data_ + slot_offset, size);
        tuple->data_ = target;
        tuple->tuple_size_ = size;
    } else {
        // 行已被删除，返回空数据
        tuple->data_ = nullptr;
        tuple->tuple_size_ = 0;
    }
}


auto TablePage::InsertTuple(const Tuple &tuple, RID *rid, const Transaction &transaction) -> bool {
    std::lock_guard lock(mutex_);
    if (tuple.tuple_size_ + SLOT_SIZE > free_size) {
        return false;
    }

    int cur_slot_point = PAGE_SIZE - (SLOT_SIZE * header_.num_rows);
    int new_slot_offset = cur_slot_point - SLOT_SIZE;


    int16_t mask = 0;
    mask |= HEAP_XMAX_INVALID;

    if (header_.num_rows == 0) {
        uint16_t tuple_offset = HEADER_SIZE;
        uint16_t size = tuple.tuple_size_;

        size_t offset = 0;
        memcpy(page_->data_ + new_slot_offset + offset, &tuple_offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);
        memcpy(page_->data_ + new_slot_offset + offset, &size, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        auto txId = transaction.GetTxId();
        memcpy(page_->data_ + new_slot_offset + offset, &txId, sizeof(tx_id_t));
        offset += sizeof(tx_id_t);

        tx_id_t xmax = 0;
        memcpy(page_->data_ + new_slot_offset + offset, &xmax, sizeof(tx_id_t));
        offset += sizeof(tx_id_t);

        memcpy(page_->data_ + new_slot_offset + offset, &rid->page_id_, sizeof(page_id_t));
        offset += sizeof(page_id_t);

        entry_id row_id = header_.num_rows + 1;
        memcpy(page_->data_ + new_slot_offset + offset, &row_id, sizeof(entry_id));
        offset += sizeof(entry_id);

        memcpy(page_->data_ + new_slot_offset + offset, &mask, sizeof(int16_t));

        memcpy(page_->data_ + tuple_offset, tuple.data_, tuple.tuple_size_);
    } else {
        uint16_t prev_tuple_offset = 0;
        uint16_t prev_size = 0;
        memcpy(&prev_tuple_offset, page_->data_ + cur_slot_point, sizeof(uint16_t));
        memcpy(&prev_size, page_->data_ + cur_slot_point + sizeof(uint16_t), sizeof(uint16_t));

        uint16_t now_offset = prev_tuple_offset + prev_size;
        size_t offset = 0;
        memcpy(page_->data_ + new_slot_offset + offset, &now_offset, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        memcpy(page_->data_ + new_slot_offset + offset, &tuple.tuple_size_, sizeof(uint16_t));
        offset += sizeof(uint16_t);

        auto tx_id = transaction.GetTxId();
        memcpy(page_->data_ + new_slot_offset + offset, &tx_id, sizeof(tx_id_t));
        offset += sizeof(tx_id_t);

        tx_id_t xmax = 0;
        memcpy(page_->data_ + new_slot_offset + offset, &xmax, sizeof(tx_id_t));
        offset += sizeof(tx_id_t);


        memcpy(page_->data_ + new_slot_offset + offset, &rid->page_id_, sizeof(page_id_t));
        offset += sizeof(page_id_t);

        entry_id row_id = header_.num_rows + 1;
        memcpy(page_->data_ + new_slot_offset + offset, &row_id, sizeof(entry_id));
        offset += sizeof(entry_id);

        memcpy(page_->data_ + new_slot_offset + offset, &mask, sizeof(int16_t));

        memcpy(page_->data_ + now_offset, tuple.data_, tuple.tuple_size_);
    }

    free_size -= SLOT_SIZE + tuple.tuple_size_;
    header_.num_rows += 1;
    rid->row_id_ = header_.num_rows;
    page_->is_dirty_ = true;
    meta_page_->UpdateTableRows(table_id_, 1);
    return true;
}

auto TablePage::SetXMax(const RID &rid, tx_id_t xmax) -> void {
    std::lock_guard lock(mutex_);
    int slot_offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    memcpy(page_->data_ + slot_offset + XMAX_OFFSET, &xmax, sizeof(tx_id_t));
    page_->is_dirty_ = true;
}

auto TablePage::updateTuple(const RID &rid, const RID &new_rid, Transaction &transaction) -> void {
    std::lock_guard lock(mutex_);
    int slot_offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    tx_id_t xmin = 0;
    memcpy(&xmin, page_->data_ + slot_offset + XMIN_OFFSET, sizeof(tx_id_t));

    int16_t mask = 0;
    memcpy(&mask, page_->data_ + slot_offset + INFO_MASK_OFFSET, sizeof(int16_t));

    if (!transaction.GetSnapShot().IsVisible(xmin) || mask & HEAP_UPDATED || mask & HEAP_XMAX_INVALID) {
        return;
    }

    tx_id_t tx_id = transaction.GetTxId();
    memcpy(page_->data_ + slot_offset + XMAX_OFFSET, &tx_id, sizeof(tx_id_t));

    memcpy(page_->data_ + slot_offset + NEW_RID_OFFSET, &new_rid.page_id_, sizeof(page_id_t));
    memcpy(page_->data_ + slot_offset + NEW_RID_OFFSET + sizeof(page_id_t), &new_rid.row_id_, sizeof(row_id_t));

    memcpy(&mask, page_->data_ + slot_offset + INFO_MASK_OFFSET, sizeof(int16_t));
    mask |= HEAP_UPDATED;
    memcpy(page_->data_ + slot_offset + INFO_MASK_OFFSET, &mask, sizeof(int16_t));
    page_->is_dirty_ = true;
}


auto TablePage::updateTuple(const Tuple &tuple, const RID &rid, const Transaction &transaction) -> void {
    std::lock_guard lock(mutex_);
    int slot_offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;
    Slot slot{};
    memcpy(&slot.offset, page_->data_ + slot_offset, sizeof(uint16_t));
    memcpy(&slot.size, page_->data_ + slot_offset + sizeof(uint16_t), sizeof(uint16_t));


    size_t prev_size = 0;
    for (size_t i = 1; i < rid.row_id_; ++i) {
        size_t offset = PAGE_SIZE - i * SLOT_SIZE + sizeof(uint16_t);
        uint16_t size = 0;
        memcpy(&size, page_->data_ + offset, sizeof(uint16_t));
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
        memmove(page_->data_ + slot.offset + slot.size, page_->data_ + slot.offset + slot.size + diff_size, prev_size);
        memcpy(page_->data_ + slot.offset, &tuple.data_, tuple.tuple_size_);
    }
}


auto TablePage::GetCount(const Transaction &transaction) const -> uint32_t {
    return header_.num_rows;
}


auto TablePage::DeleteTuple(const RID &rid, const Transaction &transaction) -> void {
    std::lock_guard lock(mutex_);

    int offset = PAGE_SIZE - rid.row_id_ * SLOT_SIZE;

    int16_t info_mask = 0;
    memcpy(&info_mask, page_->data_ + offset + INFO_MASK_OFFSET, sizeof(int16_t));

    if (!(info_mask & HEAP_XMAX_INVALID)) {
        return;
    }
    info_mask &= (info_mask | HEAP_XMAX_INVALID);

    memcpy(&page_->data_ + offset + INFO_MASK_OFFSET, &info_mask, sizeof(int16_t));
    meta_page_->UpdateTableRows(table_id_, -1);
    page_->is_dirty_ = true;
}
