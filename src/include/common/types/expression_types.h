//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <cstdint>

namespace YourSQL {

    enum class ExpressionType : uint8_t {
        STAR = 1,
        COLUMN_REF = 2,
        SELECT = 3,

        AND = 4, OR = 5,
    };


}
