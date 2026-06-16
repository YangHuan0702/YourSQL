//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include <vector>

#include "common/type.h"

namespace YourSQL {

    class TransactionManager;  // 前向声明，避免循环包含

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

        // 用于查询事务最终状态（提交/中止）；不持有所有权
        TransactionManager *txn_manager_{nullptr};

        [[nodiscard]] auto IsVisible(tx_id_t version_trx_id) const -> bool;

    };

}
