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
        COMP = 4,
        CONST = 5,
    };


    enum class OperatorType : uint8_t {
        INVALID,
        AND,
        OR,

        GT,
        GTE,
        LT,
        LTE,
        EQ,
        NEQ,

        IN,
        NOT_IN,
    };
}
