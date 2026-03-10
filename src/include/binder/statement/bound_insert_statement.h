//
// Created by huan.yang on 2026-03-09.
//
#pragma once
#include "bound_statement.h"

namespace YourSQL {

    class BoundInsertStatement: public BoundStatement {
    public:
        explicit BoundInsertStatement() : BoundStatement(StatementType::INSERT) {}
        ~BoundInsertStatement() override = default;

        auto to_string() -> std::string override {
            return "BoundInsertStatement";
        }

        entry_id table_id_{};
        std::vector<entry_id> column_ids_{};
        std::vector<Value> values_{};
    };

}
