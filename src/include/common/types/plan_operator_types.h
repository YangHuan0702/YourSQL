//
// Created by 杨欢 on 2026/2/13.
//
#pragma once

namespace YourSQL {

    enum class PlanOperatorType {
        SCAN,
        FILTER,
        PROJECTION,
        JOIN,
        AGGREGATE,
        SORT,
        LIMIT
    };

    enum class UnaryOp {
        IS_NULL,
        NOT_NULL
    };

    enum class BinaryOp {
        ADD,
        SUB,
        MU,
        DE,
        GT,
        GTE,
        LT,
        LTE,
        EQ,
        NEQ,
        LIKE,
        NLIKE,
        IN,
        CANCAT,


        AND,
        OR,
    };

}
