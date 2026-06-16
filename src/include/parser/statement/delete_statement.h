//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>
#include <string>

#include "parser/expression.h"
#include "parser/statement.h"

namespace YourSQL {
    class DeleteStatement : public BaseStatement {
    public:
        explicit DeleteStatement() : BaseStatement(
            StatementType::DELETE, StatementClassify::WHERE) {
        }
        ~DeleteStatement() override = default;

        auto to_string() -> std::string override {
            return "DeleteStatement";
        }

        auto SetWhereExpr(std::unique_ptr<BaseExpression> &where) -> void {
            this->where_expression_ = std::move(where);
        }

        std::string table_name_{};
        std::unique_ptr<BaseExpression> where_expression_{};
    };
}
