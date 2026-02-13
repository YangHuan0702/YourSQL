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

        auto GetSelectList() -> std::vector<std::unique_ptr<BaseExpression>> & {
            return selectList;
        }
        auto SetTable(std::unique_ptr<YourTable> &table) -> void {
            this->table_ = std::move(table);
        }
        auto SetWhereExpr(std::unique_ptr<BaseExpression> &newWhere) -> void{
            this->whereExpression = std::move(newWhere);
        }

        std::vector<std::unique_ptr<BaseExpression>> selectList;
        std::unique_ptr<YourTable> table_;
        std::unique_ptr<BaseExpression> whereExpression;
    };

}
