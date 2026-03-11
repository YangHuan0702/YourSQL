//
// Created by huan.yang on 2026-03-03.
//

#pragma once
#include "buffer/page.h"
#include "common/type.h"

namespace YourSQL {
    class DiskManger {
    public:
        DiskManger() = default;

        virtual ~DiskManger() = default;

        virtual auto Size() -> size_t = 0;

        virtual auto Open() -> void = 0;

        virtual auto Close() -> void = 0;

        virtual auto Read(page_id_t page_id, Page *page) -> void = 0;

        virtual auto Write(Page *page) -> void = 0;
    };
}
