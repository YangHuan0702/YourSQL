//
// Created by huan.yang on 2026-01-28.
//
#include <iostream>

#include "common/types/expression_types.h"
#include "parser/transformer.h"
#include "sql/SelectStatement.h"
using namespace YourSQL;

auto Transformer::transformSelect(hsql::SelectStatement *sqlStatement) -> std::unique_ptr<SelectStatement> {
    if (!sqlStatement) { return nullptr;}

    for (auto item : *sqlStatement->selectList) {
        // switch (item->type) {
        //     case hsql::kExprStar:
        //     case hsql::kExprColumnRef:
        //     default:
        //         return nullptr;
        // }
        std::cout << item->alias << std::endl;
    }
    return nullptr;
}

