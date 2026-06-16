//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <memory>

#include "common/types/isolation_level.h"
#include "transaction/transaction.h"

namespace YourSQL {

    // 一个客户端连接的会话状态：当前事务、autocommit、默认隔离级别。
    class ClientSession {
    public:
        ClientSession() = default;
        ~ClientSession() = default;

        auto InExplicitTxn() const -> bool { return current_txn_ != nullptr; }

        auto GetTxn() const -> std::shared_ptr<Transaction> { return current_txn_; }
        auto SetTxn(std::shared_ptr<Transaction> txn) -> void { current_txn_ = std::move(txn); }
        auto ClearTxn() -> void { current_txn_ = nullptr; }

        bool autocommit_{true};
        IsolationLevel isolation_level_{IsolationLevel::REPEATABLE_READ};

    private:
        std::shared_ptr<Transaction> current_txn_;
    };

}
