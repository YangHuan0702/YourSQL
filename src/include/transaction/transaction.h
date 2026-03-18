//
// Created by huan.yang on 2026-03-17.
//

#pragma once
#include "snapshot.h"
#include "common/type.h"
#include "common/types/isolation_level_type.h"
#include "common/types/tx_types.h"

namespace YourSQL {
    /**
    * t_infomask:
        - HEAP_XMIN_COMMITTED: xmin 已提交
        - HEAP_XMIN_ABORTED: xmin 已回滚
        - HEAP_XMAX_COMMITTED: xmax 已提交
        - HEAP_XMAX_ABORTED: xmax 已回滚
        - HEAP_XMAX_INVALID: xmax 无效
        - HEAP_UPDATED: 该 tuple 被更新过
 */
#define HEAP_XMIN_COMMITTED 1
#define HEAP_XMIN_ABORTED (1<< 1)
#define HEAP_XMAX_COMMITTED (1 << 2)
#define HEAP_XMAX_ABORTED (1 << 3)
#define HEAP_XMAX_INVALID (1 << 4)
#define HEAP_UPDATED (1 << 5)

    class Transaction {
    public:
        explicit Transaction(tx_id_t tx_id, TransactionState state, SnapShot snap_shot, IsolationLevel isolation_level);

        ~Transaction();


        auto GetTxId() const -> tx_id_t { return tx_id_; }
        auto GetState() const -> TransactionState { return state_; }
        auto GetSnapShot() const -> SnapShot { return snapshot_; }

    private:
        tx_id_t tx_id_;
        TransactionState state_;
        SnapShot snapshot_;
        IsolationLevel isolation_level_;
    };
}
