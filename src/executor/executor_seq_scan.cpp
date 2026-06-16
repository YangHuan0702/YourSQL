//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_seq_scan.h"

#include <algorithm>

#include "common/constant.h"
#include "storage/page/row.h"

using namespace YourSQL;

namespace {
    // 从一条记录的原始字节中解析 MVCC 头部
    struct RecMeta {
        tx_id_t trx_id;
        uint16_t flags;
        UndoPointer roll_ptr;
    };

    auto ParseHeader(const char *data) -> RecMeta {
        RecMeta m{};
        memcpy(&m.trx_id, data + REC_TRX_OFFSET, sizeof(tx_id_t));
        memcpy(&m.roll_ptr.page_id_, data + REC_ROLLPTR_OFFSET, sizeof(page_id_t));
        memcpy(&m.roll_ptr.slot, data + REC_ROLLPTR_OFFSET + sizeof(page_id_t), sizeof(uint32_t));
        memcpy(&m.flags, data + REC_FLAGS_OFFSET, sizeof(uint16_t));
        return m;
    }
}

auto ExecutorSeqScan::Close() -> void {
    if (iterator_) {
        delete iterator_;
        iterator_ = nullptr;
    }
}

// 判断给定 trx_id 对当前事务是否可见
auto ExecutorSeqScan::IsVisible(tx_id_t version_trx_id) -> bool {
    if (version_trx_id == INVALID_TX_ID) {
        // 无事务写入（例如未开启事务的旧数据），视为可见
        return true;
    }
    auto &txn = context_->transaction_;
    if (txn->tx_id_ == version_trx_id) {
        return true;  // 自己写的版本对自己可见
    }
    return txn->read_view_->IsVisible(version_trx_id);
}

auto ExecutorSeqScan::Next(Tuple *tuple) -> bool {
    while (iterator_ && !iterator_->IsEnd()) {
        Tuple physical = **iterator_;
        ++(*iterator_);

        if (physical.data_ == nullptr) {
            continue;
        }

        // 被回滚置 dead 的记录对所有事务不可见，直接跳过
        if (ParseHeader(physical.data_).flags & RECORD_DEAD) {
            continue;
        }

        // 无事务上下文：退化为只过滤删除标记
        if (!context_->transaction_ || !context_->transaction_->read_view_) {
            RecMeta m = ParseHeader(physical.data_);
            if (m.flags & RECORD_DEL) {
                continue;
            }
            *tuple = physical;
            tuple->schema_ = schema_;
            return true;
        }

        // 沿版本链寻找对当前快照可见的版本
        Tuple current = physical;
        bool found = false;
        bool skip_row = false;
        while (true) {
            RecMeta m = ParseHeader(current.data_);
            bool del = (m.flags & RECORD_DEL) != 0;

            if (IsVisible(m.trx_id)) {
                if (del) {
                    // 删除对本快照可见 → 该行已删除，跳过
                    skip_row = true;
                }
                found = !del;
                break;
            }

            // 当前版本不可见，回溯到 roll_ptr 指向的历史版本
            if (m.roll_ptr.IsNull() || context_->undo_log_manager_ == nullptr) {
                skip_row = true;  // 没有更旧的可见版本
                break;
            }
            current = context_->undo_log_manager_->GetUndoRecord(m.roll_ptr);
            if (current.data_ == nullptr) {
                skip_row = true;
                break;
            }
        }

        if (skip_row || !found) {
            continue;
        }

        *tuple = current;
        tuple->schema_ = schema_;
        return true;
    }
    return false;
}

auto ExecutorSeqScan::Open() -> void {
    cursor_ = 0;
    if (context_->catalog_->table_name_idx_.find(table_name_) == context_->catalog_->table_name_idx_.end()) {
        throw std::runtime_error("TableIterator::Open Table name not found");
    }
    auto table_id = context_->catalog_->table_name_idx_[table_name_];
    auto &table = context_->catalog_->tables_[table_id];
    std::transform(table->columns_.begin(),table->columns_.end(),std::back_inserter(schema_.columns_),[](const auto &p) {
        return p.second;
    });
    schema_.tuple_size_ = -1;
    iterator_ = new TableIterator(context_->buffer_manager_,context_->meta_page_,table_name_,table_id,schema_);
}
