//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "bound_expression.h"
#include "catalog/catalog.h"

namespace YourSQL {

    class BoundColumnRefExpression :public BoundExpression{
    public:
        explicit BoundColumnRefExpression(entry_id table_id,entry_id column_id) : BoundExpression(ColumnTypes::VARCHAR), table_id_(table_id),column_id_(column_id) {
        }
        ~BoundColumnRefExpression() override = default;

        auto to_string() -> std::string override {
            return "["+std::to_string(table_id_)+"."+std::to_string(column_id_)+"]";
        }
        entry_id table_id_;
        entry_id column_id_;
    };

}
