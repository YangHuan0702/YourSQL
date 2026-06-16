//
// Created by huan.yang on 2026-03-23.
//
#include "transaction/recovery_manager.h"

#include <cstring>

#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "log/log_payload.h"
#include "storage/log_disk_manager.h"
#include "storage/page/table_page.h"
#include "transaction/transaction.h"
#include "transaction/transaction_manager.h"

using namespace YourSQL;

namespace {
    // 与 LogManager::AppendLogRecord 写入格式一致的精确头大小
    const size_t kLogHeaderBytes =
        sizeof(lsn_t) + sizeof(tx_id_t) + sizeof(LogRecordType) + sizeof(uint32_t);
}

auto RecoveryManager::ReadLog() -> std::vector<RecoveredLog> {
    std::vector<RecoveredLog> out;
    std::vector<char> buf = log_disk_->ReadAll();
    size_t off = 0;
    while (off + kLogHeaderBytes <= buf.size()) {
        RecoveredLog r;
        size_t p = off;
        memcpy(&r.lsn, buf.data() + p, sizeof(lsn_t));            p += sizeof(lsn_t);
        memcpy(&r.tx_id, buf.data() + p, sizeof(tx_id_t));        p += sizeof(tx_id_t);
        memcpy(&r.type, buf.data() + p, sizeof(LogRecordType));   p += sizeof(LogRecordType);
        uint32_t psize = 0;
        memcpy(&psize, buf.data() + p, sizeof(uint32_t));         p += sizeof(uint32_t);
        if (p + psize > buf.size()) break;  // 末尾残缺记录，丢弃
        r.payload.assign(buf.data() + p, buf.data() + p + psize);
        p += psize;
        out.push_back(std::move(r));
        off = p;
    }
    return out;
}

auto RecoveryManager::Analysis(const std::vector<RecoveredLog> &logs) -> void {
    // 第一遍：标记每个事务的终态；记录最大 tx_id
    std::unordered_map<tx_id_t, TransactionState> states;
    tx_id_t max_txid = 0;

    for (const auto &log : logs) {
        if (log.tx_id > max_txid) max_txid = log.tx_id;

        switch (log.type) {
            case LogRecordType::COMMIT:
                states[log.tx_id] = TransactionState::COMMITTED;
                break;
            case LogRecordType::ABORT:
                states[log.tx_id] = TransactionState::ABORTED;
                break;
            case LogRecordType::INSERT_ROW:
            case LogRecordType::DELETE_ROW:
            case LogRecordType::UPDATE_ROW:
            case LogRecordType::BEGIN:
                // 首次见到该事务且尚无终态 -> 暂记为未提交（IN_PROGRESS）
                if (states.find(log.tx_id) == states.end()) {
                    states[log.tx_id] = TransactionState::IN_PROGRESS;
                }
                break;
            default:
                break;
        }
    }

    // 把终态写回事务管理器：已提交保持 committed；崩溃前未见 commit 的视为 aborted
    for (auto &[txid, st] : states) {
        if (txid == INVALID_TX_ID) continue;
        if (st == TransactionState::COMMITTED) {
            txn_mgr_->SetTxnState(txid, TransactionState::COMMITTED);
        } else {
            // 未提交 / 已中止 -> 标记为 ABORTED，可见性据此过滤其版本
            txn_mgr_->SetTxnState(txid, TransactionState::ABORTED);
        }
    }
    txn_mgr_->AdvanceNextTxId(max_txid);
}

auto RecoveryManager::Redo(const std::vector<RecoveredLog> &logs) -> void {
    for (const auto &log : logs) {
        if (log.type != LogRecordType::INSERT_ROW &&
            log.type != LogRecordType::DELETE_ROW &&
            log.type != LogRecordType::UPDATE_ROW) {
            continue;
        }
        LogDataPayload p = LogDataPayload::Decode(log.payload.data(),
                                                  static_cast<uint32_t>(log.payload.size()));
        if (p.rid.page_id_ == INVALID_PAGE_ID) continue;

        // 取目标页；页不存在则跳过（force 策略下已提交页应已在盘）
        Page *page = nullptr;
        try {
            page = buffer_->FetchPage(p.rid.page_id_);
        } catch (...) {
            continue;
        }
        if (page == nullptr) continue;

        TablePage tp(meta_, p.table_id, page, true);
        // 幂等：仅当页 LSN 落后于该记录时才需要重做
        if (tp.GetLsn() < log.lsn) {
            if (log.type == LogRecordType::DELETE_ROW) {
                // 重做删除标记（trx_id 用记录所属事务）
                tp.MarkDelete(p.rid, log.tx_id, UndoPointer{});
            }
            // INSERT_ROW：force 策略下数据已落盘，此处仅推进 LSN
            tp.SetLsn(log.lsn);
            buffer_->Flush(page->id_);
        }
        buffer_->Release(page->id_);
    }
}

auto RecoveryManager::Recover() -> size_t {
    std::vector<RecoveredLog> logs = ReadLog();
    if (logs.empty()) {
        return 0;
    }
    Analysis(logs);
    Redo(logs);
    return logs.size();
}
