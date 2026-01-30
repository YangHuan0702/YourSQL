//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <cstdint>

namespace YourSQL {

    enum class StatementType : uint8_t {


        SELECT = 1,
        UPDATE = 2,
        DELETE = 3,
        INSERT = 4,

        OBJ = 5,

        OPERATOR = 6,
        COMP = 7,
    };


    enum class StatementClassify : uint8_t {
        PROJECTION = 1,
        FROM = 2,
        WHERE = 3,
        GROUP = 4,
        HAVING = 5,
        LIMIT = 6,

        COLUMN = 21,
        TABLE = 22,
        FUNCTION = 23,


        OPERATOR = 100,
        OPT_AND = 101,
        OPT_OR = 102,
        EQ = 103,
        NE = 104,
        GT = 105,
        GE = 106,
        LT = 107,
        LE = 108,

    };

}
