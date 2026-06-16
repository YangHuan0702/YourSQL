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

    class BufferManager;  // 前向声明
    class MetaPage;
    class LogManager;

    class TransactionManager {
    public:
        explicit TransactionManager() = default;
        ~TransactionManager() = default;

        // 注入存储句柄，供 Abort 物理回滚使用（不持有所有权）
        auto SetStorage(BufferManager *buffer, MetaPage *meta) -> void {
            buffer_ = buffer;
            meta_ = meta;
        }

        // 注入日志管理器，commit/abort 写事务控制日志（不持有所有权）
        auto SetLogManager(LogManager *log) -> void {
            log_ = log;
        }

        auto GetNextTxId() -> tx_id_t {
            return next_txid_.fetch_add(1);
        }

        auto Begin() -> std::shared_ptr<Transaction> ;
        auto Commit(tx_id_t) -> void;
        auto Abort(tx_id_t tx_id) -> void;
        auto GetState(tx_id_t id) const -> std::optional<TransactionState> {
            return txn_state_.find(id) == txn_state_.end() ? std::nullopt : std::optional(txn_state_.at(id));
        }

        // 某事务是否已提交（用于 MVCC 可见性判断；未知 id 视为未提交）
        auto IsCommitted(tx_id_t id) const -> bool {
            auto it = txn_state_.find(id);
            return it != txn_state_.end() && it->second == TransactionState::COMMITTED;
        }

        // 某事务是否已中止
        auto IsAborted(tx_id_t id) const -> bool {
            auto it = txn_state_.find(id);
            return it != txn_state_.end() && it->second == TransactionState::ABORTED;
        }

        auto CreateReadView(tx_id_t id) -> std::shared_ptr<ReadView>;

        // 恢复支持：从日志重建事务终态与下一个事务 id
        auto SetTxnState(tx_id_t id, TransactionState state) -> void {
            std::lock_guard<std::mutex> guard(mutex_);
            txn_state_[id] = state;
        }
        auto AdvanceNextTxId(tx_id_t at_least) -> void {
            tx_id_t cur = next_txid_.load();
            if (at_least + 1 > cur) {
                next_txid_.store(at_least + 1);
            }
        }


    private:
        auto RollbackWrites(const std::shared_ptr<Transaction> &txn) -> void;

        std::atomic<tx_id_t> next_txid_{1};
        std::mutex mutex_;
        std::unordered_map<tx_id_t, std::shared_ptr<Transaction>> active_txns;
        std::unordered_map<tx_id_t,TransactionState> txn_state_;
        BufferManager *buffer_{nullptr};
        MetaPage *meta_{nullptr};
        LogManager *log_{nullptr};
    };

}
