//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "common/types/column_types.h"
#include "parser/expression.h"
#include "parser/statement.h"

namespace YourSQL {
    // 一条 SET column = value 子句
    struct UpdateSetClause {
        std::string column_name_;
        Value value_;
    };

    class UpdateStatement : public BaseStatement {
    public:
        explicit UpdateStatement() : BaseStatement(
            StatementType::UPDATE, StatementClassify::WHERE) {
        }
        ~UpdateStatement() override = default;

        auto to_string() -> std::string override {
            return "UpdateStatement";
        }

        auto SetWhereExpr(std::unique_ptr<BaseExpression> &where) -> void {
            this->where_expression_ = std::move(where);
        }

        std::string table_name_{};
        std::vector<UpdateSetClause> set_clauses_{};
        std::unique_ptr<BaseExpression> where_expression_{};
    };
}
