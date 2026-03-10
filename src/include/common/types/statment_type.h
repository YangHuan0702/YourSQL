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

        PROJECTION,
        FROM ,
        WHERE,
        GROUP,
        HAVING,
        LIMIT,

        COLUMN,
        TABLE ,
        FUNCTION,


        OPERATOR,
        OPT_AND ,
        OPT_OR ,
        EQ,
        NE ,
        GT ,
        GE ,
        LT ,
        LE ,

        INSERT,
    };

}
