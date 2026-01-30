//
// Created by huan.yang on 2026-01-30.
//
#pragma once
#include "parser/statement.h"
#include "sql/Expr.h"

namespace YourSQL {

    class OperatorStatement : public BaseStatement {
    public:
        explicit OperatorStatement(StatementType type,StatementClassify classify) : BaseStatement(type,classify) {}
        virtual ~OperatorStatement() = default;

        auto to_string() -> std::string;

        std::unique_ptr<BaseStatement> left;
        std::unique_ptr<BaseStatement> right;
    };

}
