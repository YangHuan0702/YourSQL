//
// Created by huan.yang on 2026-02-12.
//
#include "parser/transformer.h"

using namespace YourSQL;

auto Transformer::transformOperator(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    return nullptr;
}
