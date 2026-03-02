//
// Created by huan.yang on 2026-03-02.
//
#include "binder/binder.h"

using namespace YourSQL;

auto Binder::BoundIsNullExpression(std::unique_ptr<BaseExpression> &parser_where_expression) -> std::unique_ptr<BoundExpression> {
    return nullptr;
}
