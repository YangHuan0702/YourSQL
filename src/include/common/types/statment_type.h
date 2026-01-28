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
    };

}
