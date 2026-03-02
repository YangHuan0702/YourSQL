//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include <atomic>

#include "common/type.h"

namespace YourSQL {
    class IdUtil {
    public:
        auto GetNextEntryId() -> entry_id {
            return obj_id_.fetch_add(1);
        }
        std::atomic<entry_id> obj_id_{0};
    };

    class IdManager {
    public:
        static auto GetNextEntryId() -> entry_id {
            return id_util_.GetNextEntryId();
        }
        inline static IdUtil id_util_;
    };
}
