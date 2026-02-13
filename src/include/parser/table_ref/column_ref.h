//
// Created by huan.yang on 2026-02-12.
//
#pragma once
#include <string>
#include <memory>

#include "common/types/column_types.h"
#include "parser/expression.h"

namespace YourSQL {
    class ColumnRef {
    public:
        explicit ColumnRef(std::string &columnName, ColumnTypes type) : column_name(std::move(columnName)),
                                                                        column_type(type) {
        }

        std::string column_name;
        ColumnTypes column_type;
        std::unique_ptr<BaseExpression> default_expression;
    };
}
