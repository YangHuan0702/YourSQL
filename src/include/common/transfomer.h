//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <vector>

#include "sql/CreateStatement.h"
#include "parser/statement.h"

namespace YourSQL {
    class Transfomer {
    public:
        Transfomer(std::vector<hsql::SQLStatement> &statements,std::vector<std::unique_ptr<BaseStatement>> &targetStatements);
    };
}
