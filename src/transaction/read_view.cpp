//
// Created by huan.yang on 2026-03-19.
//
#include "transaction/read_view.h"

#include <algorithm>

#include "common/constant.h"
#include "transaction/transaction_manager.h"

using namespace YourSQL;


auto ReadView::IsVisible(tx_id_t version_trx_id) const -> bool {
    // 无事务写入（未开启事务的旧数据），视为可见
    if (version_trx_id == INVALID_TX_ID) {
        return true;
    }

    // 自己写的版本对自己可见
    if (version_trx_id == create_trx_id_) {
        return true;
    }

    // 已中止事务写的版本对任何人都不可见
    if (txn_manager_ != nullptr && txn_manager_->IsAborted(version_trx_id)) {
        return false;
    }

    // 高水位之上：快照之后才开始的事务，不可见
    if (version_trx_id >= low_limit_id_) {
        return false;
    }

    // 快照创建时仍活跃的事务，不可见
    if (std::find(active_ids_.begin(), active_ids_.end(), version_trx_id) != active_ids_.end()) {
        return false;
    }

    // 低水位之下：早于快照且不活跃。低水位以下默认已结束。
    if (version_trx_id < up_limit_id_) {
        // 若能查到状态且为中止，则不可见；否则（已提交或无记录的历史数据）可见
        if (txn_manager_ != nullptr && txn_manager_->IsAborted(version_trx_id)) {
            return false;
        }
        return true;
    }

    // 落在 [up_limit, low_limit) 且不在活跃集合：要求该事务已提交才可见
    if (txn_manager_ != nullptr) {
        return txn_manager_->IsCommitted(version_trx_id);
    }
    return true;
}
