//
// Created by huan.yang on 2026-03-19.
//
#include "./transaction/undo_log_manager.h"

using namespace YourSQL;


auto UndoLogManager::AppendUndoRecord(const Tuple &old_tuple, tx_id_t old_trx_id, UndoPointer old_ptr) -> UndoPointer {
    std::lock_guard guard(mutex_);

    Page *page = nullptr;
    UndoLogPage undo_page(nullptr);

    if (undo_page_id_ == INVALID_PAGE_ID) {
        page = buffer_->NewPage();
        undo_page = UndoLogPage(page);
        undo_page.InitNew(page->id_);
        undo_page_id_ = page->id_;
    } else {
        page = buffer_->FetchPage(undo_page_id_);
        undo_page = UndoLogPage(page);
        undo_page.Load();
    }

    // 当前页空间不足，分配新页
    if (!undo_page.HasSpaceFor(old_tuple.tuple_size_)) {
        buffer_->Release(page->id_);
        page = buffer_->NewPage();
        undo_page = UndoLogPage(page);
        undo_page.InitNew(page->id_);
        undo_page_id_ = page->id_;
    }

    uint32_t offset = 0;
    if (!undo_page.AppendRecord(old_ptr, old_trx_id, old_tuple.data_,
                                old_tuple.tuple_size_, &offset)) {
        buffer_->Release(page->id_);
        throw std::runtime_error("UndoLogManager::AppendUndoRecord fail.");
    }

    page_id_t result_page = page->id_;
    buffer_->Release(page->id_);
    return {result_page, offset};
}


auto UndoLogManager::GetUndoRecord(UndoPointer point) -> Tuple {
    std::lock_guard guard(mutex_);
    Page *page = buffer_->FetchPage(point.page_id_);
    if (page == nullptr) {
        throw std::runtime_error("UndoLogManager::GetUndoRecord target page is nullptr. page : " +
                                 std::to_string(point.page_id_));
    }

    UndoLogPage undo_page(page);
    undo_page.Load();

    UndoLogRecord record{};
    undo_page.ReadRecord(point.slot, &record);

    // 用独立缓冲构造 Tuple，避免指向页缓冲或栈内存导致悬空
    Tuple tuple{};
    tuple.tuple_size_ = static_cast<uint16_t>(record.payload_size_);
    if (record.payload_size_ > 0) {
        tuple.data_ = new char[record.payload_size_];
        memcpy(tuple.data_, record.payload_.data(), record.payload_size_);
    } else {
        tuple.data_ = nullptr;
    }

    buffer_->Release(page->id_);
    return tuple;
}
