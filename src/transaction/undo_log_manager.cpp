//
// Created by huan.yang on 2026-03-19.
//
#include "./transaction/undo_log_manager.h"

using namespace YourSQL;


auto UndoLogManager::AppendUndoRecord(const Tuple &old_tuple, tx_id_t old_trx_id, UndoPointer old_ptr) -> UndoPointer {
    uint32_t size = sizeof(UndoPointer) + sizeof(tx_id_t) + sizeof(uint32_t) + old_tuple.tuple_size_;

    if (undo_page_id_ == INVALID_PAGE_ID) {
        Page *page = buffer_->NewPage();

    }

    Page *page  = buffer_->FetchPage(undo_page_id_);

}


auto UndoLogManager::GetUndoRecord(UndoPointer) -> Tuple {

}

