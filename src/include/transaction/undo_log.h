//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include "common/constant.h"
#include "common/type.h"
#include "storage/r_id.h"
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
    };


    class UndoLogRecord {
    public:
        undo_id_t undo_id_{INVALID_UNDO_ID};
        undo_id_t prev_undo_id_{INVALID_UNDO_ID};

        tx_id_t trx_id_{INVALID_TX_ID};
        UndoType undo_type_{};

        entry_id table_id_;
        RID rid_;

        tx_id_t old_trx_id_{INVALID_TX_ID};
        undo_id_t old_roll_ptr_{INVALID_UNDO_ID};
        uint16_t old_flags_{0};

        Tuple before_image_;
    };
}
