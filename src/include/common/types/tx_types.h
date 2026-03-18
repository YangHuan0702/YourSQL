//
// Created by huan.yang on 2026-03-17.
//
#pragma once

namespace YourSQL {

    enum class TransactionState {
        IN_PROGRESS,
        COMMITTED,
        ABORTED
    };

}