//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "common/types/expression_types.h"
#include <string>

namespace YourSQL {

    class BaseExpression {
    public:
        explicit BaseExpression(ExpressionType type) : type(type) {}
        virtual ~BaseExpression()  = default;

        virtual auto to_string() -> std::string = 0;

        ExpressionType type;
    };

}
