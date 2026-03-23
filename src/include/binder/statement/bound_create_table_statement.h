//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include "bound_statement.h"
#include "catalog/table_entry.h"

namespace YourSQL {

    class BoundCreateTableStatement : public BoundStatement {
    public:
        explicit BoundCreateTableStatement(entry_id table_id) : BoundStatement(StatementType::CREATE_TABLE), table_id_(table_id) {}

        ~BoundCreateTableStatement() override = default;

        auto to_string() -> std::string override {
            return "BoundCreateTableStatement(" + std::to_string(table_id_) + ")";
        }

        entry_id table_id_;
    };

}
