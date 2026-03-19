//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <cstdint>

namespace YourSQL {


    enum class IsolationLevel: uint8_t {
        READ_UNCOMMITTED,
        READ_COMMITTED,
        REPEATABLE_READ,
        SERIALIZED,
    };

}
