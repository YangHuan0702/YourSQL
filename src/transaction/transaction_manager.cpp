//
// Created by huan.yang on 2026-03-19.
//
#include "transaction/transaction_manager.h"

#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "log/log_manager.h"
#include "common/types/log_record_type.h"
#include "storage/page/table_page.h"

using namespace YourSQL;

namespace {
    // 写一条无 payload 的事务控制日志
    auto WriteTxnControlLog(LogManager *log, tx_id_t tx_id, LogRecordType type) -> void {
        if (log == nullptr) return;
        LogRecordHeader header{};
        header.tx_id_ = tx_id;
        header.type_ = type;
        header.payload_size_ = 0;
        LogRecord record(header, nullptr);
        log->AppendLogRecord(record);
    }
}

auto TransactionManager::Begin() -> std::shared_ptr<Transaction> {
    auto transaction = std::make_shared<Transaction>();
    {
        std::lock_guard<std::mutex> guard(mutex_);
        transaction->tx_id_ = GetNextTxId();
        transaction->state_ = TransactionState::IN_PROGRESS;

        active_txns[transaction->tx_id_]  = transaction;
        txn_state_[transaction->tx_id_] = TransactionState::IN_PROGRESS;
    }

    transaction->read_view_ = CreateReadView(transaction->tx_id_);
    return transaction;
}

auto TransactionManager::Commit(tx_id_t id) -> void {
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = active_txns.find(id);
        if (it == active_txns.end()) throw std::runtime_error("TransactionManager::Commit transaction not found: " + std::to_string(id));
        it->second->state_ = TransactionState::COMMITTED;
        txn_state_[id] = TransactionState::COMMITTED;
        active_txns.erase(id);
    }
    // force-log-at-commit：写 COMMIT 记录并刷盘
    if (log_ != nullptr) {
        WriteTxnControlLog(log_, id, LogRecordType::COMMIT);
        log_->Flush();
    }
}


auto TransactionManager::Abort(tx_id_t tx_id) -> void {
    std::shared_ptr<Transaction> txn;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        auto it = active_txns.find(tx_id);
        if (it == active_txns.end()) throw std::runtime_error("TransactionManager::Abort transaction not found: " + std::to_string(tx_id));
        txn = it->second;
    }

    // 物理回滚写集（不持 manager 锁，避免与 buffer 锁嵌套）
    RollbackWrites(txn);

    {
        std::lock_guard<std::mutex> guard(mutex_);
        txn->state_ = TransactionState::ABORTED;
        txn_state_[tx_id] = TransactionState::ABORTED;
        active_txns.erase(tx_id);
    }
    // 写 ABORT 记录并刷盘，使恢复时该事务被判为未提交
    if (log_ != nullptr) {
        WriteTxnControlLog(log_, tx_id, LogRecordType::ABORT);
        log_->Flush();
    }
}

auto TransactionManager::RollbackWrites(const std::shared_ptr<Transaction> &txn) -> void {
    if (buffer_ == nullptr || meta_ == nullptr) {
        return;  // 未注入存储句柄，跳过物理回滚（可见性层仍会过滤 aborted 版本）
    }
    auto meta_shared = std::shared_ptr<MetaPage>(meta_, [](MetaPage *) {});

    // 逆序撤销，保证 update 的 new/old 顺序正确
    for (auto it = txn->write_set_.rbegin(); it != txn->write_set_.rend(); ++it) {
        const WriteRecord &wr = *it;
        Page *page = buffer_->FetchPage(wr.rid.page_id_);
        if (page == nullptr) continue;

        TablePage tp(meta_shared, wr.table_id, page, true);
        switch (wr.type) {
            case WriteType::INSERT:
            case WriteType::UPDATE_INSERT_NEW:
                tp.MarkDead(wr.rid);
                break;
            case WriteType::DELETE:
            case WriteType::UPDATE_DELETE_OLD:
                tp.RestoreRecord(wr.rid, wr.prev_trx_id, wr.prev_roll_ptr, wr.prev_flags);
                break;
        }
        buffer_->Flush(page->id_);
        buffer_->Release(page->id_);
    }
}

auto TransactionManager::CreateReadView(tx_id_t id) -> std::shared_ptr<ReadView> {
    std::lock_guard<std::mutex> guard(mutex_);

    auto read_view = std::make_shared<ReadView>();
    read_view->create_trx_id_ = id;
    read_view->txn_manager_ = this;

    read_view->low_limit_id_ = next_txid_.load();

    read_view->up_limit_id_ = read_view->low_limit_id_;

    for (const auto &[active_id,txn] : active_txns) {
        if (active_id == id) continue;

        if (txn->state_ != TransactionState::IN_PROGRESS) {
            continue;
        }

        read_view->active_ids_.push_back(active_id);
        read_view->up_limit_id_ = std::min(read_view->up_limit_id_,active_id);
    }
    return read_view;
}



