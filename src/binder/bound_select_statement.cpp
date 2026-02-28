//
// Created by 杨欢 on 2026/2/28.
//
#include "binder/binder.h"
#include "parser/expression/column_expression.h"
using namespace YourSQL;


auto Binder::BoundSelectStatement(std::unique_ptr<SelectStatement> parser_statement) -> std::unique_ptr<YourSQL::BoundSelectStatement> {

    auto ans = std::make_unique<class BoundSelectStatement>();

    // select list
    if (parser_statement->selectList.size() == 1) {
        // boundStarExpression
        const auto starExpression = BoundStarExpression(parser_statement->table_->table_name_);
        for (auto &bound_column_ref_expression : starExpression->columns_) {
            ans->select_.push_back(std::move(bound_column_ref_expression));
        }
    } else {
        for (auto &column : parser_statement->selectList) {
            std::string &columnName = dynamic_cast<ColumnExpression*>(column.get())->column_name;
            ans->select_.push_back(BoundColumnRefExpression(parser_statement->table_->table_name_,columnName));
        }
    }


    // table
    ans->table_ = BoundTableRefExpression(parser_statement->table_->table_name_);

    // TODO(where)

    return ans;
}

