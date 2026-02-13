//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/statement.h"
#include "parser/table_ref/your_table.h"

namespace YourSQL {
    class SelectStatement : public BaseStatement {
    public:
        SelectStatement() : BaseStatement(StatementType::SELECT, StatementClassify::WHERE) {}
        ~SelectStatement() override = default;

        auto to_string() -> std::string override {
            return "SelectStatement";
        }


        std::vector<std::unique_ptr<BaseExpression>> selectList;
        std::unique_ptr<YourTable> table_;
    };
}
