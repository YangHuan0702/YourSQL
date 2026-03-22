//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include "buffer/page.h"
#include "common/type.h"

namespace YourSQL {
    enum class UndoType : uint8_t {
        INSERT,
        UPDATE,
        DELETE
    };

    struct UndoPointer {
        page_id_t page_id_;
        uint32_t slot;

        auto IsNull() const -> bool { return page_id_ == 0; }
    };

    struct UndoLogRecord {
        UndoPointer old_roll_ptr_;
        tx_id_t old_trx_id_;
        uint32_t payload_size_;
        char payload_[0];
    };

    /**
     * undo-log format:
     * -----------------------------------------------------------------
     * | page_id | lsn_ | free_offset | record | record | record | ... |
     * -----------------------------------------------------------------
     *
     *
     * undo-log record format :
     * --------------------------------------------------------------------
     * | old_roll_ptr(page_id,slot) | old_trx_id | payload_size | payload |
     * --------------------------------------------------------------------
     */
    class UndoLogPage {
    public:
        explicit UndoLogPage(Page *page) : page_(page) {}
        ~UndoLogPage()  = default;

        auto Init() -> void;
        auto AppendRecord(const UndoLogRecord &record) -> bool;
        auto ReadRecord(uint32_t offset, UndoLogRecord *record) const -> void;

        auto GetFreeOffset() const -> uint32_t {
            return free_offset_;
        }


        page_id_t page_id_{};
        lsn_t lsn_{};
        uint32_t free_offset_{};
        Page *page_;
    };


}
