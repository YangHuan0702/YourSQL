//
// Created by huan.yang on 2026-03-09.
//
#pragma once
#include <string>
#include <vector>
#include "sql/Expr.h"
#include "parser/statement.h"

namespace YourSQL {
    class InsertStatement : public BaseStatement {
    public:
        explicit InsertStatement() : BaseStatement(
            StatementType::INSERT, StatementClassify::INSERT) {
        }
        ~InsertStatement() override = default;

        auto to_string() -> std::string override {
            return "InsertStatement";
        }

        std::string table_name_{};
        std::vector<std::string> column_name_{};
        std::vector<Value> values_{};
    };
}
