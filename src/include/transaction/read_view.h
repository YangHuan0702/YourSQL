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
        tx_id_t create_trx_id_;
        tx_id_t up_limit_id_;
        tx_id_t low_limit_id_;
        std::vector<tx_id_t> active_ids_;


        auto IsVisible(tx_id_t version_trx_id) const -> bool;

    };

}
