//
// Created by huan.yang on 2026-03-19.
//
#include <stdexcept>

#include "transaction/undo_log.h"

using namespace YourSQL;

auto UndoLogPage::Init() -> void {
    if (page_ == nullptr) { throw std::runtime_error("UndoLogPage::Init() page_ is nullptr."); }

    size_t offset = 0;
    memcpy(&page_id_,page_->data_ + offset,sizeof(page_id_t));
    offset += sizeof(page_id_t);

    memcpy(&lsn_,page_->data_ + offset,sizeof(lsn_t));
    offset += sizeof(lsn_t);

    memcpy(&free_offset_,page_->data_ + offset,sizeof(uint32_t));

}

auto UndoLogPage::ReadRecord(uint32_t offset, UndoLogRecord *record) const -> void {
    char *target = page_->data_ + offset;

    size_t undo_offset = 0 ;
    memcpy(&record->old_roll_ptr_.page_id_, target + undo_offset, sizeof(page_id_t));
    undo_offset += sizeof(page_id_t);
    memcpy(&record->old_roll_ptr_.slot,target + undo_offset, sizeof(uint32_t));
    undo_offset += sizeof(uint32_t);
    memcpy(&record->old_trx_id_, target + undo_offset, sizeof(tx_id_t));
    undo_offset += sizeof(tx_id_t);
    uint32_t payload_size = 0;
    memcpy(&payload_size, target + undo_offset, sizeof(uint32_t));
    record->payload_size_ = payload_size;
    undo_offset += sizeof(uint32_t);
    memcpy(&record->payload_, target + undo_offset, payload_size);
}

auto UndoLogPage::AppendRecord(const UndoLogRecord &record) -> bool {
    size_t free_size = PAGE_SIZE - free_offset_;

    size_t record_size = sizeof(UndoPointer) + sizeof(tx_id_t) + sizeof(uint32_t) + record.payload_size_;

    if (free_size < record_size) {
        throw std::runtime_error("UndoLogPage::AppendRecord() free_size < record_size.");
    }

    size_t offset= 0;
    memcpy(&page_->data_ + offset,&record.old_roll_ptr_.page_id_,sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(&page_->data_ + offset,&record.old_roll_ptr_.slot,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&page_->data_ + offset,&record.old_trx_id_,sizeof(tx_id_t));
    offset += sizeof(tx_id_t);
    memcpy(&page_->data_ + offset,&record.payload_size_,sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&page_->data_ + offset,&record.payload_,record.payload_size_);

    free_offset_ += record.payload_size_;
    page_->is_dirty_ = true;
    return true;
}

