//
// Created by huan.yang on 2026-02-12.
//
#include "parser/transformer.h"
#include "parser/expression/comp_expression.h"

using namespace YourSQL;


auto Transformer::transformAndOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    auto left = transformOperator(expr->expr);
    auto right = transformOperator(expr->expr2);
    return std::make_unique<CompExpression>(OperatorType::AND,std::move(left),std::move(right));
}

auto Transformer::transformOrOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    auto left = transformOperator(expr->expr);
    auto right = transformOperator(expr->expr2);
  return std::make_unique<CompExpression>(OperatorType::OR,std::move(left),std::move(right));
}

auto Transformer::transformOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    switch (expr->opType) {
        case hsql::kOpOr:
            return transformOrOperator(expr);
        case hsql::kOpAnd:
            return transformAndOperator(expr);
        case hsql::kOpLike:
        case hsql::kOpNotLike:
            return transformLikeExpr(expr);
        case hsql::kOpIn:
            return transformInExpr(expr);
        case hsql::kOpLess:
        case hsql::kOpLessEq :
        case hsql::kOpGreater:
        case hsql::kOpGreaterEq:
        case hsql::kOpEquals:
        case hsql::kOpNotEquals:
            return transformLogicExpr(expr);
        case hsql::kOpIsNull:
            return transformIsNullOperator(expr);
        default:
            throw std::runtime_error("don`t know the opType");
    }
}
