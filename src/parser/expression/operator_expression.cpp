//
// Created by huan.yang on 2026-02-12.
//
#include "binder/bound_binary_expression.h"
#include "common/types/plan_operator_types.h"
#include "parser/transformer.h"
#include "parser/expression/binary_expression.h"

using namespace YourSQL;


auto Transformer::transformAndOperator(hsql::Expr *expr,const std::string& table_name) -> std::unique_ptr<BaseExpression> {
    auto left = transformOperator(expr->expr,table_name);
    auto right = transformOperator(expr->expr2,table_name);
    return std::make_unique<BinaryExpression>(std::move(left),std::move(right),BinaryOp::AND);
}

auto Transformer::transformOrOperator(hsql::Expr *expr,const std::string& table_name) -> std::unique_ptr<BaseExpression> {
    auto left = transformOperator(expr->expr,table_name);
    auto right = transformOperator(expr->expr2,table_name);
  return std::make_unique<BinaryExpression>(std::move(left),std::move(right),BinaryOp::OR);
}

auto Transformer::transformOperatorType(hsql::OperatorType type) -> BinaryOp {
    switch (type) {
        case hsql::OperatorType::kOpPlus: return BinaryOp::ADD;
        case hsql::OperatorType::kOpMinus: return BinaryOp::SUB;
        case hsql::OperatorType::kOpAsterisk: return BinaryOp::MU;
        case hsql::OperatorType::kOpSlash: return BinaryOp::DE;

        case hsql::OperatorType::kOpEquals: return BinaryOp::EQ;
        case hsql::OperatorType::kOpNotEquals: return BinaryOp::NEQ;
        case hsql::OperatorType::kOpLess: return BinaryOp::LT;
        case hsql::OperatorType::kOpLessEq: return BinaryOp::LTE;
        case hsql::OperatorType::kOpGreater: return BinaryOp::GT;
        case hsql::OperatorType::kOpGreaterEq: return BinaryOp::GTE;
        case hsql::OperatorType::kOpLike: return BinaryOp::LIKE;
        case hsql::OperatorType::kOpNotLike: return BinaryOp::NLIKE;
        case hsql::OperatorType::kOpAnd: return BinaryOp::AND;
        case hsql::OperatorType::kOpOr: return BinaryOp::OR;
        case hsql::OperatorType::kOpIn: return BinaryOp::IN;
        case hsql::OperatorType::kOpConcat: return BinaryOp::CANCAT;
        default: throw std::invalid_argument("Invalid Operator Binary Type");
    }
}


auto Transformer::transformBinaryOperator(hsql::Expr *expr, const std::string &table_name) -> std::unique_ptr<BaseExpression> {
    auto left = transformOperator(expr->expr,table_name);
    auto right = transformOperator(expr->expr2,table_name);
    return std::make_unique<BinaryExpression>(std::move(left),std::move(right),transformOperatorType(expr->opType));
}


auto Transformer::transformUnaryOperator(hsql::Expr *expr, const std::string &table_name) -> std::unique_ptr<BaseExpression> {
    return nullptr;
}


auto Transformer::transformOperator(hsql::Expr *expr,const std::string& table_name) -> std::unique_ptr<BaseExpression> {
    // TODO: 从新调整
    switch (expr->opType) {
        case hsql::kOpOr:
            return transformOrOperator(expr,table_name);
        case hsql::kOpAnd:
            return transformAndOperator(expr,table_name);
        case hsql::kOpLike:
        case hsql::kOpNotLike:
        case hsql::kOpIn:
        case hsql::kOpLess:
        case hsql::kOpLessEq :
        case hsql::kOpGreater:
        case hsql::kOpGreaterEq:
        case hsql::kOpEquals:
        case hsql::kOpNotEquals:
        case hsql::kOpNot:
            return transformBinaryOperator(expr,table_name);
        case hsql::kOpIsNull:
            return transformUnaryOperator(expr,table_name);
        default:
            throw std::runtime_error("don`t know the opType");
    }
}
