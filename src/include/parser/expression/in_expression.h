//
// Created by 杨欢 on 2026/2/13.
//

#pragma once
#include "parser/expression.h"
#include "common/types/column_types.h"
#include <string>
#include <vector>

namespace YourSQL {

    class InExpression: public BaseExpression {

    public:
        explicit InExpression(const std::string &column_name, const std::vector<Value> &values): BaseExpression(ExpressionType::OPERATOR),target_column_(std::move(column_name)),values_(values) {}
        ~InExpression() override = default;
        auto to_string() -> std::string override {
            return "InExpression";
        }

        std::string target_column_;
        std::vector<Value> values_;
    };


}
