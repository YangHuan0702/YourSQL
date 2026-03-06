//
// Created by huan.yang on 2026-03-02.
//
#include "binder/bound_unary_expression.h"
#include "binder/binder.h"
#include "parser/expression/binary_expression.h"
#include "binder/bound_binary_expression.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/constant_expression.h"

using namespace YourSQL;


auto Binder::BoundCompExpression(std::unique_ptr<BaseExpression> &parser_where_expression,std::unique_ptr<class BoundTableRefExpression> &table) -> std::unique_ptr<BoundExpression> {
    if (!parser_where_expression) {
        return nullptr;
    }

    if (parser_where_expression->type == ExpressionType::COLUMN_REF) {
        auto comp_expression = dynamic_cast<ColumnExpression*> (parser_where_expression.get());
        return BoundColumnRefExpression(table->table_name_,comp_expression->column_name);
    }

    if (parser_where_expression->type == ExpressionType::CONST) {
        auto comp_expression = dynamic_cast<ConstantExpression*> (parser_where_expression.get());
        return BoundConstExpression(comp_expression->value_,comp_expression->type_);
    }

    if (parser_where_expression->type == ExpressionType::BINARY) {
        auto comp_expression = dynamic_cast<BinaryExpression*> (parser_where_expression.get());
        switch (comp_expression->op_) {
            case BinaryOp::OR:
            case BinaryOp::AND:
            case BinaryOp::LIKE:
            case BinaryOp::NLIKE:
            case BinaryOp::IN:
            case BinaryOp::LT:
            case BinaryOp::LTE :
            case BinaryOp::GT:
            case BinaryOp::GTE:
            case BinaryOp::EQ:
            case BinaryOp::NEQ:
                return BoundBinaryExpression(parser_where_expression,table);
            default:
                throw std::invalid_argument("[Binder]Invalid expr binary Type");
        }
    }

    // if (parser_where_expression->type == ExpressionType::UNARY) {
    //     auto comp_expression = dynamic_cast<UnaryExpression*> (parser_where_expression.get());
    //     switch (comp_expression->op_) {
    //         case UnaryOp::IS_NULL:
    //         case UnaryOp::NOT_NULL:
    //             return BoundUnaryExpression();
    //         default: throw std::invalid_argument("[Binder]Invalid expr unary Type");
    //     }
    // }
    throw std::invalid_argument("[Binder]Invalid expr is not binary Type");
}
