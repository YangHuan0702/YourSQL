//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include "common/type.h"
#include "storage/r_id.h"
#include "storage/page/tuple.h"

namespace YourSQL {
    enum class UndoType {
        INSERT,
        UPDATE,
        DELETE
    };

    class UndoLogRecord {
    public:
        tx_id_t trx_id_;

        Tuple old_;
        RID prev_roll_id_;

        UndoType type_;
    };
}
