//
// Created by 杨欢 on 2026/2/28.
//
#pragma once
#include "common/types/statment_type.h"
#include <string>
#include "binder/bound_expression.h"

namespace YourSQL {

    class BoundStatement {
    public:
        explicit BoundStatement(StatementType type) : type_(type) {}
        virtual ~BoundStatement() = default;

        virtual auto to_string() -> std::string = 0;

        StatementType type_;
        std::vector<std::unique_ptr<BoundExpression>> condition_;
    };

}
