//
// Created by huan.yang on 2026-03-22.
//
#include "executor/executor_update.h"

#include <algorithm>

#include "common/constant.h"
#include "common/macro.h"
#include "storage/page/row.h"

using namespace YourSQL;

auto ExecutorUpdate::IsVisible(tx_id_t version_trx_id) -> bool {
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

auto ExecutorUpdate::Open() -> void {
    table_name_ = context_->catalog_->GetTableName(table_id_);
    auto &table = context_->catalog_->tables_[table_id_];
    std::transform(table->columns_.begin(), table->columns_.end(),
                   std::back_inserter(schema_.columns_),
                   [](const auto &p) { return p.second; });
    schema_.tuple_size_ = -1;
    iterator_ = new TableIterator(context_->buffer_manager_, context_->meta_page_,
                                  table_name_, table_id_, schema_);
}

namespace {
    // 把新行插入表尾，必要时分配并链接新页。返回插入位置的 RID。
    auto InsertNewVersion(const std::shared_ptr<ExecutorContext> &context, entry_id table_id,
                          const Tuple &tuple) -> RID {
        MetaItem &item = context->meta_page_->items_[table_id];

        Page *page = nullptr;
        bool read = true;
        if (item.last_page_id == INVALID_PAGE_ID) {
            page = context->buffer_manager_->NewPage();
            read = false;
            context->meta_page_->UpdateTableLastId(table_id, page->id_);
            if (item.first_page_id == INVALID_PAGE_ID) {
                context->meta_page_->UpdateTableFirstId(table_id, page->id_);
            }
        } else {
            page = context->buffer_manager_->FetchPage(item.last_page_id);
        }

        auto *table_page = new TablePage(context->meta_page_, table_id, page, read);

        if (!table_page->HasSpaceFor(tuple.tuple_size_)) {
            // 分配新页并链接
            page_id_t old_id = table_page->GetPage()->id_;
            Page *new_page = context->buffer_manager_->NewPage();
            page_id_t new_id = new_page->id_;
            table_page->SetNextPageId(new_id);
            context->buffer_manager_->Flush(old_id);
            context->buffer_manager_->Release(old_id);
            delete table_page;
            table_page = new TablePage(context->meta_page_, table_id, new_page, false);
            context->meta_page_->UpdateTableLastId(table_id, new_id);
        }

        RID rid{};
        table_page->InsertTuple(tuple, &rid);
        lsn_t lsn = context->LogDataChange(LogRecordType::INSERT_ROW, table_id, rid,
                                           tuple.data_, tuple.tuple_size_);
        if (lsn != INVALID_LSN) {
            table_page->SetLsn(lsn);
        }
        context->buffer_manager_->Flush(table_page->GetPage()->id_);
        context->buffer_manager_->Release(table_page->GetPage()->id_);
        delete table_page;
        return rid;
    }
}

auto ExecutorUpdate::Next(Tuple *tuple) -> bool {
    (void) tuple;
    if (done_) {
        return false;
    }

    tx_id_t cur_txn = context_->transaction_ ? context_->transaction_->tx_id_ : INVALID_TX_ID;

    // 阶段一：收集命中行（RID、旧 trx_id、旧 roll_ptr、旧行原始字节、新值集合）
    struct Hit {
        RID rid;
        tx_id_t old_trx;
        UndoPointer old_roll_ptr;
        uint16_t old_flags;            // 旧行写操作前的 flags（回滚恢复用）
        std::vector<char> old_bytes;   // 旧版本完整记录字节（用于 undo）
        std::vector<Value> new_values; // 应用 SET 后的新值
    };
    std::vector<Hit> hits;

    while (iterator_ && !iterator_->IsEnd()) {
        RID rid = iterator_->GetRID();
        TablePage *page = iterator_->GetCurrentPage();
        uint16_t flags = page->ReadRecordFlags(rid);
        tx_id_t version_trx = page->ReadRecordTrxId(rid);

        bool already_deleted = (flags & RECORD_DEL) != 0;
        if (!already_deleted && IsVisible(version_trx)) {
            Tuple old_tuple = **iterator_;
            old_tuple.schema_ = schema_;

            bool matched = true;
            if (filter_) {
                Value v = filter_->Evaluate(old_tuple);
                matched = !v.IsNull() && v.GetBool();
            }

            if (matched) {
                // 写写冲突检测（first-updater-wins）
                if (context_->transaction_manager_ && version_trx != INVALID_TX_ID &&
                    version_trx != cur_txn) {
                    auto st = context_->transaction_manager_->GetState(version_trx);
                    if (st.has_value() && st.value() == TransactionState::IN_PROGRESS) {
                        delete[] old_tuple.data_;
                        throw std::runtime_error("WriteConflict: row modified by another in-progress transaction");
                    }
                }

                Row row(schema_);
                row.Deserialize(old_tuple);
                Hit hit;
                hit.rid = rid;
                hit.old_trx = version_trx;
                hit.old_roll_ptr = page->ReadRecordRollPtr(rid);
                hit.old_flags = flags;
                hit.old_bytes.assign(old_tuple.data_, old_tuple.data_ + old_tuple.tuple_size_);
                hit.new_values = row.values_;
                // 应用 SET：按 column_id 覆盖新值
                for (const auto &set_clause : set_clauses_) {
                    if (set_clause.column_id_ < hit.new_values.size()) {
                        hit.new_values[set_clause.column_id_] = set_clause.value_;
                    }
                }
                hits.push_back(std::move(hit));
            }
            delete[] old_tuple.data_;
        }
        ++(*iterator_);
    }

    // 阶段二：对每个命中行执行 delete-old + insert-new，形成版本链
    for (auto &hit : hits) {
        // 1) 旧版本写入 undo，得到新行要指向的 roll_ptr
        UndoPointer new_roll_ptr{INVALID_PAGE_ID, 0};
        if (context_->undo_log_manager_) {
            Tuple old_payload;
            old_payload.data_ = hit.old_bytes.data();
            old_payload.tuple_size_ = static_cast<uint16_t>(hit.old_bytes.size());
            new_roll_ptr = context_->undo_log_manager_->AppendUndoRecord(
                old_payload, hit.old_trx, hit.old_roll_ptr);
        }

        // 2) 序列化新版本行：携带新值、当前事务 id、指向旧版本的 roll_ptr
        Row new_row(schema_, hit.new_values);
        new_row.SetTrxId(cur_txn);
        new_row.SetFlags(0);
        new_row.SetRollPtr(new_roll_ptr);

        char *data = new_row.Serialize();
        Tuple new_tuple;
        new_tuple.data_ = data;
        new_tuple.tuple_size_ = new_row.use_size_;
        new_tuple.schema_ = schema_;

        // 3) 在原页标记删除旧版本（xmax=当前事务，roll_ptr 指向 undo）
        {
            Page *p = context_->buffer_manager_->FetchPage(hit.rid.page_id_);
            TablePage tp(context_->meta_page_, table_id_, p, true);
            tp.MarkDelete(hit.rid, cur_txn, new_roll_ptr);
            lsn_t lsn = context_->LogDataChange(LogRecordType::DELETE_ROW, table_id_, hit.rid, nullptr, 0);
            if (lsn != INVALID_LSN) {
                tp.SetLsn(lsn);
            }
            context_->buffer_manager_->Flush(p->id_);
            context_->buffer_manager_->Release(p->id_);
        }
        if (context_->transaction_) {
            WriteRecord wr_old;
            wr_old.rid = hit.rid;
            wr_old.type = WriteType::UPDATE_DELETE_OLD;
            wr_old.table_id = table_id_;
            wr_old.prev_trx_id = hit.old_trx;
            wr_old.prev_roll_ptr = hit.old_roll_ptr;
            wr_old.prev_flags = hit.old_flags;
            context_->transaction_->write_set_.push_back(wr_old);
        }

        // 4) 插入新版本到表尾
        RID new_rid = InsertNewVersion(context_, table_id_, new_tuple);
        if (context_->transaction_) {
            WriteRecord wr_new;
            wr_new.rid = new_rid;
            wr_new.type = WriteType::UPDATE_INSERT_NEW;
            wr_new.table_id = table_id_;
            context_->transaction_->write_set_.push_back(wr_new);
        }
        delete[] data;
        ++updated_count_;
    }

    done_ = true;
    return false;
}

auto ExecutorUpdate::Close() -> void {
    if (iterator_) {
        delete iterator_;
        iterator_ = nullptr;
    }
}
