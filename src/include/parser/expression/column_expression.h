//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "common/types/expression_types.h"
#include "parser/expression.h"

namespace YourSQL {
    class ColumnExpression : public BaseExpression {
    public:
        ColumnExpression(std::string &column_name, std::string &alias) : BaseExpression(ExpressionType::COLUMN_REF),
                                                                         column_name(std::move(column_name)),
                                                                         alias(std::move(alias)) {
        }

        ~ColumnExpression() override = default;

        auto to_string() -> std::string override;

        std::string column_name;

        std::string alias;
    };
}
