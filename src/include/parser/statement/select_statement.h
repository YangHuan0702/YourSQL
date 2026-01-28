//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "parser/statement.h"

namespace YourSQL {
    class SelectStatement : public BaseStatement {
    public:
        SelectStatement() : BaseStatement(StatementType::SELECT, StatementClassify::WHERE) {}
    };
}
