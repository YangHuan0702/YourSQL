//
// Created by huan.yang on 2026-03-19.
//
#include "transaction/transaction_manager.h"

using namespace YourSQL;

auto TransactionManager::Begin() -> std::shared_ptr<Transaction> {
    auto transaction = std::make_shared<Transaction>();
    {
        std::lock_guard<std::mutex> guard(mutex_);
        transaction->tx_id_ = GetNextTxId();
        transaction->state_ = TransactionState::IN_PROGRESS;

        active_txns[transaction->tx_id_]  = transaction;
        txn_state_[transaction->tx_id_] = TransactionState::IN_PROGRESS;
    }

    transaction->read_view_ = CreateReadView(transaction->tx_id_);
    return transaction;
}

auto TransactionManager::Commit(tx_id_t id) -> void {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = active_txns.find(id);
    if (it == active_txns.end()) throw std::runtime_error("TransactionManager::Commit transaction not found: " + std::to_string(id));
    it->second->state_ = TransactionState::COMMITTED;
    txn_state_[id] = TransactionState::COMMITTED;
    active_txns.erase(id);
}


auto TransactionManager::Abort(tx_id_t tx_id) -> void {
    std::lock_guard<std::mutex> guard(mutex_);

    auto it = active_txns.find(tx_id);
    if (it == active_txns.end()) throw std::runtime_error("TransactionManager::Commit transaction not found: " + std::to_string(tx_id));

    it->second->state_ = TransactionState::ABORTED;
    txn_state_[tx_id] = TransactionState::ABORTED;
    active_txns.erase(tx_id);
}

auto TransactionManager::CreateReadView(tx_id_t id) -> std::shared_ptr<ReadView> {
    std::lock_guard<std::mutex> guard(mutex_);

    auto read_view = std::make_shared<ReadView>();
    read_view->create_trx_id_ = id;

    read_view->low_limit_id_ = next_txid_.load();

    read_view->up_limit_id_ = read_view->low_limit_id_;

    for (const auto &[active_id,txn] : active_txns) {
        if (active_id == id) continue;

        if (txn->state_ != TransactionState::IN_PROGRESS) {
            continue;
        }

        read_view->active_ids_.push_back(active_id);
        read_view->up_limit_id_ = std::min(read_view->up_limit_id_,active_id);
    }
    return read_view;
}



