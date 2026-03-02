//
// Created by huan.yang on 2026-03-02.
//
#include "binder/bound_comp_expression.h"
#include "binder/binder.h"
#include "parser/expression/comp_expression.h"
#include "parser/expression/logic_expression.h"

using namespace YourSQL;

auto Binder::BoundOrExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    auto or_expression = dynamic_cast<CompExpression *>(parser_where_expression.get());
    auto left = BoundCompExpression(or_expression->left_);
    auto right = BoundCompExpression(or_expression->right_);

    auto ans = std::make_unique<class BoundCompExpression>(OperatorType::OR,ColumnTypes::BOOL);
    if (left) {
        ans->AddChildren(std::move(left));
    }
    if (right) {
        ans->AddChildren(std::move(right));
    }
    return ans;
}

auto Binder::BoundAndExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    auto and_expression = dynamic_cast<CompExpression *>(parser_where_expression.get());
    auto left = BoundCompExpression(and_expression->left_);
    auto right = BoundCompExpression(and_expression->right_);

    auto ans = std::make_unique<class BoundCompExpression>(OperatorType::AND,ColumnTypes::BOOL);
    if (left) {
        ans->AddChildren(std::move(left));
    }
    if (right) {
        ans->AddChildren(std::move(right));
    }
    return ans;
}

auto Binder::BoundCompExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    if (!parser_where_expression) {
        return nullptr;
    }
    if (parser_where_expression->type == ExpressionType::EXPR) {
        auto comp_expression = dynamic_cast<CompExpression*> (parser_where_expression.get());
        switch (comp_expression->operator_type) {
            case OperatorType::OR:
                return BoundOrExpression(parser_where_expression);
            case OperatorType::AND:
                return BoundAndExpression(parser_where_expression);
            default:
                throw std::invalid_argument("[Binder]Invalid expr Type");
        }
    }
    if (parser_where_expression->type == ExpressionType::OPERATOR) {
        auto logic_expression = dynamic_cast<LogicExpression*> (parser_where_expression.get());
        switch (logic_expression->type_) {
            case OperatorType::LIKE:
            case OperatorType::LIKEN:
                return BoundLikeExpression(parser_where_expression);
            case OperatorType::IN:
                return BoundInExpression(parser_where_expression);
            case OperatorType::LT:
            case OperatorType::LTE :
            case OperatorType::GT:
            case OperatorType::GTE:
            case OperatorType::EQ:
            case OperatorType::NEQ:
                return BoundLogicalCompExpression(parser_where_expression);
            case OperatorType::ISN:
                return BoundIsNullExpression(parser_where_expression);
            default:
                throw std::runtime_error("[Binder]don`t know the opType");
        }
    }
    throw std::invalid_argument("[Binder]Invalid Operator Type");
}
