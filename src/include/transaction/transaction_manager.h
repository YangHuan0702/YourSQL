//
// Created by 杨欢 on 2026/3/15.
//

#pragma once
#include <mutex>
#include <set>
#include <unordered_map>

#include "common/type.h"
#include "common/types/tx_types.h"

namespace YourSQL {
    class TransactionManager {
    public:
        explicit TransactionManager();
        ~TransactionManager() = default;

        auto Begin() -> tx_id_t;
        auto Commit(tx_id_t tx_id) -> void;
        auto Abort(tx_id_t tx_id) -> void;
        auto GetTransactionState(tx_id_t) -> TransactionState;
        auto CreateSnapshot() -> void;

    private:
        auto GetNextTxId() -> tx_id_t;

        std::unordered_map<tx_id_t,TransactionState> txn_state_table_;
        std::set<tx_id_t> active_txns_;
        std::mutex mutex_;
    };
}
