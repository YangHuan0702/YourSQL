//
// Created by huan.yang on 2026-03-19.
//
#include <stdexcept>

#include "transaction/undo_log.h"
#include "common/macro.h"

using namespace YourSQL;

auto UndoLogPage::WriteHeader() -> void {
    if (page_ == nullptr) { throw std::runtime_error("UndoLogPage::WriteHeader page_ is nullptr."); }
    size_t offset = 0;
    memcpy(page_->data_ + offset, &page_id_, sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(page_->data_ + offset, &lsn_, sizeof(lsn_t));
    offset += sizeof(lsn_t);
    memcpy(page_->data_ + offset, &free_offset_, sizeof(uint32_t));
    page_->is_dirty_ = true;
}

auto UndoLogPage::InitNew(page_id_t page_id) -> void {
    if (page_ == nullptr) { throw std::runtime_error("UndoLogPage::InitNew page_ is nullptr."); }
    page_id_ = page_id;
    lsn_ = 0;
    // 首条记录从页头之后开始
    free_offset_ = kHeaderSize;
    WriteHeader();
}

auto UndoLogPage::Load() -> void {
    if (page_ == nullptr) { throw std::runtime_error("UndoLogPage::Load page_ is nullptr."); }

    size_t offset = 0;
    memcpy(&page_id_, page_->data_ + offset, sizeof(page_id_t));
    offset += sizeof(page_id_t);
    memcpy(&lsn_, page_->data_ + offset, sizeof(lsn_t));
    offset += sizeof(lsn_t);
    memcpy(&free_offset_, page_->data_ + offset, sizeof(uint32_t));

    // 兼容尚未初始化的页（free_offset 为 0 说明页头还没写过）
    if (free_offset_ < kHeaderSize) {
        free_offset_ = kHeaderSize;
    }
}

auto UndoLogPage::HasSpaceFor(uint32_t payload_size) const -> bool {
    uint32_t record_size = sizeof(page_id_t) + sizeof(uint32_t) + sizeof(tx_id_t)
                           + sizeof(uint32_t) + payload_size;
    return free_offset_ + record_size <= PAGE_SIZE;
}

auto UndoLogPage::ReadRecord(uint32_t offset, UndoLogRecord *record) const -> void {
    const char *target = page_->data_ + offset;

    size_t undo_offset = 0;
    memcpy(&record->old_roll_ptr_.page_id_, target + undo_offset, sizeof(page_id_t));
    undo_offset += sizeof(page_id_t);
    memcpy(&record->old_roll_ptr_.slot, target + undo_offset, sizeof(uint32_t));
    undo_offset += sizeof(uint32_t);
    memcpy(&record->old_trx_id_, target + undo_offset, sizeof(tx_id_t));
    undo_offset += sizeof(tx_id_t);
    uint32_t payload_size = 0;
    memcpy(&payload_size, target + undo_offset, sizeof(uint32_t));
    undo_offset += sizeof(uint32_t);
    record->payload_size_ = payload_size;
    record->payload_.resize(payload_size);
    if (payload_size > 0) {
        memcpy(record->payload_.data(), target + undo_offset, payload_size);
    }
}

auto UndoLogPage::AppendRecord(const UndoPointer &old_roll_ptr, tx_id_t old_trx_id,
                               const char *payload, uint32_t payload_size,
                               uint32_t *out_offset) -> bool {
    if (!HasSpaceFor(payload_size)) {
        return false;
    }

    uint32_t offset = free_offset_;
    char *base = page_->data_ + offset;
    size_t pos = 0;
    memcpy(base + pos, &old_roll_ptr.page_id_, sizeof(page_id_t));
    pos += sizeof(page_id_t);
    memcpy(base + pos, &old_roll_ptr.slot, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    memcpy(base + pos, &old_trx_id, sizeof(tx_id_t));
    pos += sizeof(tx_id_t);
    memcpy(base + pos, &payload_size, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    if (payload_size > 0) {
        memcpy(base + pos, payload, payload_size);
        pos += payload_size;
    }

    free_offset_ += static_cast<uint32_t>(pos);
    WriteHeader();
    page_->is_dirty_ = true;

    if (out_offset != nullptr) {
        *out_offset = offset;
    }
    return true;
}
