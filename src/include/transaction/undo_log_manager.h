//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <memory>

#include "undo_log.h"
#include "buffer/buffer_manager.h"

namespace YourSQL {

    class UndoLogManager {
    public:
        explicit UndoLogManager(std::shared_ptr<BufferManager> buffer) : buffer_(std::move(buffer)) {}
        ~UndoLogManager() = default;

        auto AppendUndoRecord(const Tuple &old_tuple,tx_id_t old_trx_id, UndoPointer old_ptr) -> UndoPointer;

        auto GetUndoRecord(UndoPointer) -> Tuple;

    private:
        std::shared_ptr<BufferManager> buffer_;
        page_id_t undo_page_id_{INVALID_PAGE_ID};
    };

}
