//
// Created by huan.yang on 2026-01-30.
//
#pragma once
#include <memory>

#include "parser/statement.h"
#include <string>

#include "parser/expression.h"
#include "parser/expression/column_expression.h"

namespace YourSQL {

    class CompStatement : public BaseStatement {
    public :
        CompStatement(StatementClassify classify) : BaseStatement(StatementType::COMP,classify) {}
        ~CompStatement() = default;

        auto to_string() -> std::string;

    private:
        std::unique_ptr<ColumnExpression> column;
    };

}
