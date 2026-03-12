//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include <atomic>

#include "common/type.h"

namespace YourSQL {
    inline std::atomic<entry_id> obj_id_{1};

    class IdManager {
    public:
        static auto GetNextEntryId() -> entry_id {
            return obj_id_.fetch_add(1);
        }
    };
}
