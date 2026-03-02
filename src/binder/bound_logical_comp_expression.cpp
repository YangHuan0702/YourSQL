//
// Created by huan.yang on 2026-03-02.
//
#include "binder/binder.h"
#include "parser/expression/logic_expression.h"
#include "binder/bound_comp_expression.h"

using namespace YourSQL;

auto Binder::BoundLogicalCompExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    auto logical_expression = dynamic_cast<LogicExpression*>(parser_where_expression.get());

    switch (logical_expression->type_) {
        case OperatorType::LT:
        case OperatorType::LTE:
        case OperatorType::GT:
        case OperatorType::GTE:
        case OperatorType::EQ:
        case OperatorType::NEQ: {
            auto ans = std::make_unique<class BoundCompExpression>(logical_expression->type_,ColumnTypes::BOOL);
            ans->AddChildren(BoundColumnRefExpression(logical_expression->target_table,logical_expression->target_column));
            ans->AddChildren(BoundConstExpression(logical_expression->value));
            break;
        }
        default: throw std::invalid_argument("[Binder]Unknown operator");
    }
    return nullptr;
}
