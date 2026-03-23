//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <cstdint>

namespace YourSQL {

    enum class LogRecordType: uint8_t {
        BEGIN,
        COMMIT,
        ABORT,
        INSERT_ROW,
        UPDATE_ROW,
        DELETE_ROW,
        INSERT_UNDO,
        PAGE_NEW,
        CHECKPOINT
    };

}
