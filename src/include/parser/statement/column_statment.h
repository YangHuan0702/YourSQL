//
// Created by huan.yang on 2026-01-30.
//
#pragma once
#include "parser/statement.h"
#include <string>


namespace YourSQL {

    class ColumnStatement : public BaseStatement {
    public:
        ColumnStatement() : BaseStatement(StatementType::OBJ,StatementClassify::COLUMN) {}
        ~ColumnStatement() = default;

        auto to_string() -> std::string;


    };

}
