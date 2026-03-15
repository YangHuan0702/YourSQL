//
// Created by 杨欢 on 2026/3/15.
//
#pragma once
#include <cstdint>

namespace YourSQL {

    enum class IndexPageType: uint8_t {
        INVALID = 0,
        INTERVAL = 1,
        LEAF = 2
    };

}
