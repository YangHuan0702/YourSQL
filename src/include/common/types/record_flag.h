//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <cstdint>

namespace YourSQL {

    enum RecordFlag : uint16_t {
        RECORD_DELETE_MARK = 1<< 0,
        RECORD_COMPACT = 1<< 1,
    };

}
