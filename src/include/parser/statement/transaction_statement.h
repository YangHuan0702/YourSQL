//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include <string>

#include "parser/statement.h"

namespace YourSQL {

    enum class TxnCommand : uint8_t {
        BEGIN,
        COMMIT,
        ROLLBACK,
    };

    // 事务控制语句：BEGIN / COMMIT / ROLLBACK
    // 不进入 binder/planner，由会话层直接驱动 TransactionManager。
    class TransactionStatement : public BaseStatement {
    public:
        explicit TransactionStatement(TxnCommand command)
            : BaseStatement(StatementType::TRANSACTION, StatementClassify::OPERATOR),
              command_(command) {
        }
        ~TransactionStatement() override = default;

        auto to_string() -> std::string override {
            switch (command_) {
                case TxnCommand::BEGIN: return "BeginTransaction";
                case TxnCommand::COMMIT: return "CommitTransaction";
                case TxnCommand::ROLLBACK: return "RollbackTransaction";
            }
            return "TransactionStatement";
        }

        TxnCommand command_;
    };
}
