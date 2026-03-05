//
// Created by huan.yang on 2026-03-02.
//
#include "binder/binder.h"
#include "parser/expression/logic_expression.h"
#include "binder/bound_unary_expression.h"
#include "binder/bound_binary_expression.h"
#include "parser/expression/binary_expression.h"

using namespace YourSQL;

auto Binder::BoundBinaryExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    auto logical_expression = dynamic_cast<BinaryExpression*>(parser_where_expression.get());
    auto ans = std::make_unique<class BoundBinaryExpression>(logical_expression->op_);
    ans->AddChildren(BoundCompExpression(logical_expression->left_));
    ans->AddChildren(BoundCompExpression(logical_expression->right_));
    return ans;
}
