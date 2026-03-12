//
// Created by huan.yang on 2026-03-12.
//
#pragma once
#include <atomic>

#include "common/type.h"

namespace YourSQL {
    inline std::atomic<page_id_t> page_id_{1};
    class PageIdUtil {
    public:
        static auto GetNextPageId() -> page_id_t {
            return page_id_.fetch_add(1);
        }
    };

}
