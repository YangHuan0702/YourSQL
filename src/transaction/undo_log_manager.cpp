//
// Created by huan.yang on 2026-03-19.
//
#include "./transaction/undo_log_manager.h"

using namespace YourSQL;


auto UndoLogManager::AppendUndoRecord(const Tuple &old_tuple, tx_id_t old_trx_id, UndoPointer old_ptr)  -> UndoPointer {
    std::lock_guard guard(mutex_);
    Page *page = nullptr;
    if (undo_page_id_ == INVALID_PAGE_ID) {
        page = buffer_->NewPage();
        page->Reset();
    } else {
        page = buffer_->FetchPage(undo_page_id_);
    }
    auto undo_page = reinterpret_cast<UndoLogPage*>(page->data_);
    undo_page->Init();

    UndoLogRecord record{};
    record.old_roll_ptr_ = old_ptr;
    record.old_trx_id_ = old_trx_id;
    record.payload_size_ = old_tuple.tuple_size_;
    memcpy(record.payload_,old_tuple.data_,old_tuple.tuple_size_);

    auto offset = undo_page->GetFreeOffset();

    if (!undo_page->AppendRecord(record)) {
        throw std::runtime_error("undo_log_manager::AppendUndoRecord fail.");
    }
    buffer_->Release(page->id_);
    return {page->id_,offset};
}


auto UndoLogManager::GetUndoRecord(UndoPointer point) -> Tuple {
    std::lock_guard guard(mutex_);
    Page *page = buffer_->FetchPage(point.page_id_);
    if (page == nullptr) {
        throw std::runtime_error("undo_log_manager::GetUndoRecord target page is nullptr. page : "+ std::to_string(point.page_id_));
    }

    auto undo_page = reinterpret_cast<UndoLogPage*>(page->data_);
    undo_page->Init();

    UndoLogRecord record{};
    undo_page->ReadRecord(point.slot,&record);

    Tuple tuple{};
    tuple.tuple_size_ = record.payload_size_;
    tuple.data_ = record.payload_;

    buffer_->Release(page->id_);
    return tuple;
}

