//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include "buffer/page.h"
#include "common/type.h"
#include "storage/page/tuple.h"

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

    class UndoLogPage {
    public:
        explicit UndoLogPage(Page *page) : page_(page) {}
        ~UndoLogPage()  = default;

        Page *page_;
    };


}
