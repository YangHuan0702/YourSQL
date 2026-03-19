//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include "type.h"

namespace YourSQL {

#define PRODUCT_NAME "YourDB"

    constexpr page_id_t META_PAGE_ID = 0;

    constexpr size_t META_INIT_VERSION = 0;
    constexpr size_t META_INIT_TABLE_SIZE = 0;
    constexpr size_t META_INIT_LAST_POINT = (sizeof(size_t) * 3);


    constexpr  tx_id_t INVALID_TX_ID = 0;
    constexpr  undo_id_t INVALID_UNDO_ID = 0;
    constexpr  lsn_t INVALID_LSN = 0;
}
