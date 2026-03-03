//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include "common/type.h"

namespace YourSQL {

    struct RID {
        page_id_t page_id_;
        row_id_t row_id_;
    };
}
