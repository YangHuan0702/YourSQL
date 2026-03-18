//
// Created by huan.yang on 2026-03-17.
//
#pragma once
#include <algorithm>
#include <set>
#include <vector>

#include "common/type.h"

namespace YourSQL {
    class SnapShot {
    public:
        explicit SnapShot(tx_id_t cmin, tx_id_t cmax, std::vector<tx_id_t> active_ids, tx_id_t cur_tx_id) : cmin_(cmin),
            cmax_(cmax),cur_tx_id_(cur_tx_id), active_ids_(active_ids) {
            std::sort(active_ids_.begin(),active_ids_.end());
        }

        ~SnapShot() = default;

        auto IsVisible(tx_id_t tx_id) -> bool {
            if (tx_id == cur_tx_id_) return true;
            if (tx_id < cmin_) return true;
            if (tx_id >= cmax_)  return false;
            return !std::binary_search(active_ids_.begin(),active_ids_.end(),tx_id);
        }

    private:
        tx_id_t cmin_;
        tx_id_t cmax_;
        tx_id_t cur_tx_id_;
        std::vector<tx_id_t> active_ids_;
    };
}
