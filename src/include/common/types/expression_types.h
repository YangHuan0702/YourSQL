//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <cstdint>

namespace YourSQL {
    enum class ExpressionType : uint8_t {
        INVALID = 0,
        STAR = 1,
        COLUMN_REF = 2,
        SELECT = 3,
        EXPR = 4,
        CONST = 5,
        OPERATOR = 6,
    };


    enum class OperatorType : uint8_t {
        INVALID,
        AND,
        OR,
        ISN,
        GT,
        GTE,
        LT,
        LTE,
        EQ,
        NEQ,

        LIKE,
        LIKEN,
        IN,
        NOT_IN,
    };
}
