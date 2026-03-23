//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include <vector>
#include <memory>

#include "read_view.h"
#include "undo_log.h"
#include "common/constant.h"
#include "common/type.h"
#include "common/types/isolation_level.h"
#include "storage/r_id.h"

namespace YourSQL {
    enum class TransactionState:uint8_t {
        COMMITTED,
        ABORTED,
        IN_PROGRESS,
        ACTIVE
    };

    class Transaction {

    public:
        tx_id_t tx_id_{INVALID_TX_ID};
        TransactionState state_{TransactionState::IN_PROGRESS};
        IsolationLevel isolation_level_{IsolationLevel::REPEATABLE_READ};

        bool autocommit_{false};
        bool read_only_{false};

        std::shared_ptr<ReadView> read_view_;
        std::vector<UndoPointer> undo_ids_;
        std::vector<RID> write_set_;
    };
}
