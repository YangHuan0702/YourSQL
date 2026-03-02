//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <memory>

#include "bound_column_ref_expression.h"
#include "bound_const_expression.h"
#include "bound_star_expression.h"
#include "bound_table_ref_expression.h"
#include "parser/expression.h"
#include "parser/statement/select_statement.h"
#include "statement/bound_select_statement.h"

namespace YourSQL {

    class Binder {
    public :
        explicit Binder(std::shared_ptr<Catalog> catalog) : catalog_(std::move(catalog)) {}
        ~Binder() = default;

        auto BoundSelectStatement(std::unique_ptr<SelectStatement> parser_statement) -> std::unique_ptr<BoundSelectStatement>;

        auto BoundStarExpression(std::string &table_name) -> std::unique_ptr<BoundStarExpression>;
        auto BoundColumnRefExpression(std::string &table_name, std::string &column_name) -> std::unique_ptr<BoundColumnRefExpression>;
        auto BoundTableRefExpression(std::string &table_name) -> std::unique_ptr<BoundTableRefExpression>;

        auto BoundCompExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundOrExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundAndExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundLikeExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundInExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundLogicalCompExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundIsNullExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression>;
        auto BoundConstExpression(Value &value) -> std::unique_ptr<BoundExpression> {
            return std::make_unique<YourSQL::BoundConstExpression>(value);
        }
        std::shared_ptr<Catalog> catalog_;
    };

}
