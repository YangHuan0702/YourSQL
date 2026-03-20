//
// Created by huan.yang on 2026-03-19.
//
#include "transaction/transaction_manager.h"

using namespace YourSQL;

auto TransactionManager::Begin() -> std::shared_ptr<Transaction> {
    auto transaction = std::make_shared<Transaction>();
    transaction->tx_id_ = GetNextTxId();
    transaction->state_ = TransactionState::IN_PROGRESS;

    active_txns[transaction->tx_id_] = transaction;
    return transaction;
}

auto TransactionManager::Commit(tx_id_t) -> void {

}


auto TransactionManager::Abort(tx_id_t tx_id) -> void {

}

