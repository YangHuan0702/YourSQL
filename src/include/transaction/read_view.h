//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include <vector>

#include "common/type.h"

namespace YourSQL {

    class ReadView {
    public:
        explicit ReadView() = default;
        ~ReadView() = default;

        // 创建这个 read view 的事务 ID
        tx_id_t create_trx_id_;

        // 低水位，小于它的事务对该 view 一定可见
        tx_id_t up_limit_id_;

        // 高水位，大于等于它的事务对该 view 一定不可见
        tx_id_t low_limit_id_;

        // 拍快照时仍活跃的读写事务集合
        std::vector<tx_id_t> active_ids_;


        auto IsVisible(tx_id_t version_trx_id) const -> bool;

    };

}
