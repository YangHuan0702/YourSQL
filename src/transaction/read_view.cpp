//
// Created by huan.yang on 2026-03-19.
//
#include "transaction/read_view.h"

#include <algorithm>

#include "common/constant.h"

using namespace YourSQL;


auto ReadView::IsVisible(tx_id_t version_trx_id) const -> bool {
    if (version_trx_id == INVALID_TX_ID) {
        return true;
    }

    if (version_trx_id == create_trx_id_) {
        return true;
    }

    if (version_trx_id < up_limit_id_) {
        return true;
    }

    if (version_trx_id >= low_limit_id_) {
        return false;
    }

    return std::find(active_ids_.begin(),active_ids_.end(),version_trx_id) == active_ids_.end();
}
