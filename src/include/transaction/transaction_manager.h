//
// Created by 杨欢 on 2026/3/15.
//

#pragma once
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "transaction.h"
#include "common/type.h"

namespace YourSQL {

    class TransactionManager {
    public:
        explicit TransactionManager() = default;
        ~TransactionManager() = default;


        auto GetNextTxId() -> tx_id_t {
            return next_txid_.fetch_add(1);
        }

        auto Begin() -> tx_id_t ;
        auto Commit(tx_id_t) -> void;
        auto Abort() -> void;


    private:
        std::atomic<tx_id_t> next_txid_{1};
        std::mutex mutex_;
        std::unordered_map<tx_id_t, std::shared_ptr<Transaction>> active_txns;
        std::unordered_map<tx_id_t,TransactionState> txn_state_;
    };

}
