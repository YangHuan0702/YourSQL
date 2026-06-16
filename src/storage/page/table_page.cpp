//
// Created by huan.yang on 2026-03-03.
//
#include "storage/page/table_page.h"

#include "common/constant.h"
#include "storage/page/row.h"

using namespace YourSQL;


TablePage::TablePage(std::shared_ptr<MetaPage> meta_page,entry_id table_id,Page *page,bool read) : meta_page_(std::move(meta_page)),table_id_(table_id),page_(page),free_size(PAGE_SIZE - HEADER_SIZE) {
    char *data = page->data_;
    if (read) {
        size_t header_offset = 0;
        memcpy(&header_.version, data + header_offset, sizeof(uint16_t));
        header_offset += sizeof(uint16_t);
        memcpy(&header_.num_rows,data+NUM_ROWS_OFFSET,sizeof(uint32_t));
        header_offset += sizeof(uint32_t);

        memcpy(&header_.page_id,data+header_offset,sizeof(page_id_t));
        header_offset += sizeof(page_id_t);

        memcpy(&header_.next_page_id,data+header_offset,sizeof(page_id_t));
        header_offset += sizeof(page_id_t);

        memcpy(&header_.lsn_,data+header_offset,sizeof(lsn_t));

        uint32_t slot_count_size = SLOT_SIZE * header_.num_rows;
        uint32_t tuple_count_size = 0;
        for (uint32_t i = 1; i <= header_.num_rows; ++i) {
            size_t offset = PAGE_SIZE - i * SLOT_SIZE + sizeof(uint16_t);
            uint16_t size = 0;
            memcpy(&size, data+offset, sizeof(uint16_t));
            tuple_count_size += size;
        }
        free_size -= slot_count_size + tuple_count_size;
    } else {
        // init
        header_.version = 0;
        header_.num_rows = 0;
        header_.page_id = page->id_;
        header_.next_page_id = 0;
        header_.lsn_ = 0;

        WriteHeader();
        page_->is_dirty_ = true;
    }
}

auto TablePage::WriteHeader() -> void {
    char *data = page_->data_;
    size_t offset = 0;
    memcpy(data + offset, &header_.version, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    memcpy(data + offset, &header_.num_rows, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(data + offset, &header_.page_id, sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(data + offset, &header_.next_page_id, sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(data + offset, &header_.lsn_, sizeof(lsn_t));
    page_->is_dirty_ = true;
}

auto TablePage::SlotOffset(row_id_t row_id) const -> size_t {
    return PAGE_SIZE - row_id * SLOT_SIZE;
}

auto TablePage::SetNextPageId(page_id_t next_page_id) -> void {
    std::lock_guard lock(mutex_);
    header_.next_page_id = next_page_id;
    WriteHeader();
}

auto TablePage::SetLsn(lsn_t lsn) -> void {
    std::lock_guard lock(mutex_);
    header_.lsn_ = lsn;
    WriteHeader();
}

auto TablePage::HasSpaceFor(uint16_t tuple_size) const -> bool {
    return tuple_size + SLOT_SIZE <= free_size;
}


auto TablePage::GetTuple(const RID &rid, Tuple *tuple) -> void {
    std::lock_guard lock(mutex_);
    size_t offset = SlotOffset(rid.row_id_);

    uint16_t slot_offset = 0;
    uint16_t size = 0;
    memcpy(&slot_offset,page_->data_+offset,sizeof(uint16_t));
    memcpy(&size,page_->data_+offset+sizeof(uint16_t),sizeof(uint16_t));

    // 返回包含记录头的原始 tuple 数据；可见性/删除判断交给上层 MVCC 逻辑
    auto target = new char[size];
    memcpy(target,page_->data_+slot_offset,size);
    tuple->data_ = target;
    tuple->tuple_size_ = size;
}

auto TablePage::ReadRecordTrxId(const RID &rid) -> tx_id_t {
    std::lock_guard lock(mutex_);
    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));
    tx_id_t trx_id = 0;
    memcpy(&trx_id, page_->data_ + slot_offset + REC_TRX_OFFSET, sizeof(tx_id_t));
    return trx_id;
}

auto TablePage::ReadRecordFlags(const RID &rid) -> uint16_t {
    std::lock_guard lock(mutex_);
    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));
    uint16_t flags = 0;
    memcpy(&flags, page_->data_ + slot_offset + REC_FLAGS_OFFSET, sizeof(uint16_t));
    return flags;
}

auto TablePage::ReadRecordRollPtr(const RID &rid) -> UndoPointer {
    std::lock_guard lock(mutex_);
    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));
    UndoPointer roll_ptr{};
    memcpy(&roll_ptr.page_id_, page_->data_ + slot_offset + REC_ROLLPTR_OFFSET, sizeof(page_id_t));
    memcpy(&roll_ptr.slot, page_->data_ + slot_offset + REC_ROLLPTR_OFFSET + sizeof(page_id_t), sizeof(uint32_t));
    return roll_ptr;
}


auto TablePage::InsertTuple(const Tuple &tuple,RID *rid) -> bool {
    std::lock_guard lock(mutex_);
    if (tuple.tuple_size_ + SLOT_SIZE > free_size) {
        return false;
    }

    int cur_slot_point = PAGE_SIZE - (SLOT_SIZE * header_.num_rows);
    int new_slot_offset = cur_slot_point - SLOT_SIZE;

    if (header_.num_rows == 0) {
        uint16_t tuple_offset = HEADER_SIZE;
        uint16_t size = tuple.tuple_size_;
        memcpy(page_->data_ + new_slot_offset, &tuple_offset, sizeof(uint16_t));
        memcpy(page_->data_ + new_slot_offset + sizeof(uint16_t), &size, sizeof(uint16_t));

        memcpy(page_->data_+tuple_offset, tuple.data_, tuple.tuple_size_);
    } else {

        uint16_t prev_tuple_offset = 0;
        uint16_t prev_size = 0;
        memcpy(&prev_tuple_offset, page_->data_ + cur_slot_point, sizeof(uint16_t));
        memcpy(&prev_size, page_->data_ + cur_slot_point + sizeof(uint16_t), sizeof(uint16_t));

        uint16_t now_offset = prev_tuple_offset + prev_size;
        memcpy(page_->data_ + new_slot_offset, &now_offset, sizeof(uint16_t));
        memcpy(page_->data_ + new_slot_offset + sizeof(uint16_t), &tuple.tuple_size_, sizeof(uint16_t));
        memcpy(page_->data_+now_offset, tuple.data_, tuple.tuple_size_);
    }

    free_size -= SLOT_SIZE + tuple.tuple_size_;
    header_.num_rows += 1;
    rid->page_id_ = header_.page_id;
    rid->row_id_ = header_.num_rows;
    WriteHeader();
    page_->is_dirty_ = true;
    meta_page_->UpdateTableRows(table_id_,1);
    return true;
}




auto TablePage::updateTuple(const Tuple &tuple, const RID &rid) -> void {
    // MVCC 约束：UPDATE 不做原地覆盖，应由执行器实现为 delete-old + insert-new。
    // 这里仅保留接口；如被调用说明上层逻辑有误。
    (void) tuple;
    (void) rid;
    throw std::runtime_error("TablePage::updateTuple: in-place update is forbidden under MVCC; use delete+insert");
}


auto TablePage::GetCount() const -> uint32_t {
    return header_.num_rows;
}


auto TablePage::MarkDelete(const RID &rid, tx_id_t trx_id, UndoPointer roll_ptr) -> void {
    std::lock_guard lock(mutex_);

    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));

    uint16_t flags = 0;
    memcpy(&flags, page_->data_ + slot_offset + REC_FLAGS_OFFSET, sizeof(uint16_t));

    if (flags & RECORD_DEL) {
        // 已被删除，幂等返回
        return;
    }

    flags |= RECORD_DEL;
    // 写删除事务 id、指向旧版本的 undo 指针、删除标记
    memcpy(page_->data_ + slot_offset + REC_TRX_OFFSET, &trx_id, sizeof(tx_id_t));
    memcpy(page_->data_ + slot_offset + REC_ROLLPTR_OFFSET, &roll_ptr.page_id_, sizeof(page_id_t));
    memcpy(page_->data_ + slot_offset + REC_ROLLPTR_OFFSET + sizeof(page_id_t), &roll_ptr.slot, sizeof(uint32_t));
    memcpy(page_->data_ + slot_offset + REC_FLAGS_OFFSET, &flags, sizeof(uint16_t));

    page_->is_dirty_ = true;
    meta_page_->UpdateTableRows(table_id_, -1);
}


auto TablePage::DeleteTuple(const RID &rid) -> void {
    // 兼容旧接口：无事务信息的删除标记
    MarkDelete(rid, INVALID_TX_ID, UndoPointer{});
}


auto TablePage::RestoreRecord(const RID &rid, tx_id_t trx_id, UndoPointer roll_ptr, uint16_t flags) -> void {
    std::lock_guard lock(mutex_);

    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));

    uint16_t old_flags = 0;
    memcpy(&old_flags, page_->data_ + slot_offset + REC_FLAGS_OFFSET, sizeof(uint16_t));

    // 覆写记录头三字段，恢复到写操作之前的状态
    memcpy(page_->data_ + slot_offset + REC_TRX_OFFSET, &trx_id, sizeof(tx_id_t));
    memcpy(page_->data_ + slot_offset + REC_ROLLPTR_OFFSET, &roll_ptr.page_id_, sizeof(page_id_t));
    memcpy(page_->data_ + slot_offset + REC_ROLLPTR_OFFSET + sizeof(page_id_t), &roll_ptr.slot, sizeof(uint32_t));
    memcpy(page_->data_ + slot_offset + REC_FLAGS_OFFSET, &flags, sizeof(uint16_t));

    // 若此前是删除标记、现在恢复为可见，则行数 +1
    if ((old_flags & RECORD_DEL) && !(flags & RECORD_DEL)) {
        meta_page_->UpdateTableRows(table_id_, 1);
    }
    page_->is_dirty_ = true;
}


auto TablePage::MarkDead(const RID &rid) -> void {
    std::lock_guard lock(mutex_);

    size_t offset = SlotOffset(rid.row_id_);
    uint16_t slot_offset = 0;
    memcpy(&slot_offset, page_->data_ + offset, sizeof(uint16_t));

    uint16_t flags = 0;
    memcpy(&flags, page_->data_ + slot_offset + REC_FLAGS_OFFSET, sizeof(uint16_t));

    bool was_live = !(flags & RECORD_DEL) && !(flags & RECORD_DEAD);
    flags |= RECORD_DEAD;
    memcpy(page_->data_ + slot_offset + REC_FLAGS_OFFSET, &flags, sizeof(uint16_t));

    if (was_live) {
        meta_page_->UpdateTableRows(table_id_, -1);
    }
    page_->is_dirty_ = true;
}
