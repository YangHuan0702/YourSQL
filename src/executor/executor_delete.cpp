//
// Created by huan.yang on 2026-03-20.
//
#include "executor/executor_delete.h"

#include <algorithm>

#include "common/constant.h"
#include "storage/page/row.h"

using namespace YourSQL;

auto ExecutorDelete::IsVisible(tx_id_t version_trx_id) -> bool {
    if (version_trx_id == INVALID_TX_ID) {
        return true;
    }
    auto &txn = context_->transaction_;
    if (!txn || !txn->read_view_) {
        return true;
    }
    if (txn->tx_id_ == version_trx_id) {
        return true;
    }
    return txn->read_view_->IsVisible(version_trx_id);
}

auto ExecutorDelete::Open() -> void {
    if (context_->catalog_->table_name_idx_.find(table_name_) ==
        context_->catalog_->table_name_idx_.end()) {
        throw std::runtime_error("ExecutorDelete::Open table not found: " + table_name_);
    }
    table_id_ = context_->catalog_->table_name_idx_[table_name_];
    auto &table = context_->catalog_->tables_[table_id_];
    std::transform(table->columns_.begin(), table->columns_.end(),
                   std::back_inserter(schema_.columns_),
                   [](const auto &p) { return p.second; });
    schema_.tuple_size_ = -1;
    iterator_ = new TableIterator(context_->buffer_manager_, context_->meta_page_,
                                  table_name_, table_id_, schema_);
}

auto ExecutorDelete::Next(Tuple *tuple) -> bool {
    (void) tuple;
    if (done_) {
        return false;
    }

    tx_id_t cur_txn = context_->transaction_ ? context_->transaction_->tx_id_ : INVALID_TX_ID;

    while (iterator_ && !iterator_->IsEnd()) {
        RID rid = iterator_->GetRID();
        TablePage *page = iterator_->GetCurrentPage();

        uint16_t flags = page->ReadRecordFlags(rid);
        tx_id_t version_trx = page->ReadRecordTrxId(rid);

        bool already_deleted = (flags & RECORD_DEL) != 0;
        if (!already_deleted && IsVisible(version_trx)) {
            // 读出旧版本整条记录及其头部信息
            Tuple old_tuple = **iterator_;

            // WHERE 过滤：不满足条件则跳过
            bool matched = true;
            if (filter_) {
                old_tuple.schema_ = schema_;
                Value v = filter_->Evaluate(old_tuple);
                matched = !v.IsNull() && v.GetBool();
            }

            if (matched) {
                // 写写冲突检测（first-updater-wins）：目标行被另一个未提交事务改过则冲突
                if (context_->transaction_manager_ && version_trx != INVALID_TX_ID &&
                    version_trx != cur_txn) {
                    auto st = context_->transaction_manager_->GetState(version_trx);
                    if (st.has_value() && st.value() == TransactionState::IN_PROGRESS) {
                        delete[] old_tuple.data_;
                        throw std::runtime_error("WriteConflict: row modified by another in-progress transaction");
                    }
                }

                UndoPointer old_roll_ptr = page->ReadRecordRollPtr(rid);

                UndoPointer new_roll_ptr{INVALID_PAGE_ID, 0};
                if (context_->undo_log_manager_) {
                    new_roll_ptr = context_->undo_log_manager_->AppendUndoRecord(
                        old_tuple, version_trx, old_roll_ptr);
                }

                page->MarkDelete(rid, cur_txn, new_roll_ptr);

                // 写 WAL（DELETE_ROW，无需 after-image，靠 rid 定位重做删除标记）
                lsn_t lsn = context_->LogDataChange(LogRecordType::DELETE_ROW, table_id_, rid, nullptr, 0);
                if (lsn != INVALID_LSN) {
                    page->SetLsn(lsn);
                }

                if (context_->transaction_) {
                    WriteRecord wr;
                    wr.rid = rid;
                    wr.type = WriteType::DELETE;
                    wr.table_id = table_id_;
                    wr.prev_trx_id = version_trx;
                    wr.prev_roll_ptr = old_roll_ptr;
                    wr.prev_flags = flags;
                    context_->transaction_->write_set_.push_back(wr);
                }
                context_->buffer_manager_->Flush(page->GetPage()->id_);
                ++deleted_count_;
            }

            delete[] old_tuple.data_;
        }

        ++(*iterator_);
    }

    done_ = true;
    return false;
}

auto ExecutorDelete::Close() -> void {
    if (iterator_) {
        delete iterator_;
        iterator_ = nullptr;
    }
}
