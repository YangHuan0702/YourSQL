//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include "catalog/catalog.h"

namespace YourSQL {

    class BoundColumnRefExpression{
    public:
        explicit BoundColumnRefExpression(entry_id table_id,entry_id column_id) : table_id_(table_id),column_id_(column_id) {
        }
        ~BoundColumnRefExpression() = default;

        auto to_string() -> std::string {
            return "["+std::to_string(table_id_)+"."+std::to_string(column_id_)+"]";
        }
        entry_id table_id_;
        entry_id column_id_;
    };

}
