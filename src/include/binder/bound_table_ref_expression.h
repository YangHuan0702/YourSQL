//
// Created by 杨欢 on 2026/2/28.
//
#pragma once
#include <string>
#include "common/type.h"

namespace YourSQL {

    class BoundTableRefExpression {
    public:
        explicit BoundTableRefExpression(entry_id table_id,std::string tableName) : table_name_(tableName),table_id_(table_id) {}
        ~BoundTableRefExpression() =default;

        auto to_string() const -> std::string {
            return std::to_string(table_id_);
        }
        std::string table_name_;
        entry_id table_id_;
    };

}
