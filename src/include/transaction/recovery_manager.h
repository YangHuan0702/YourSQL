//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "common/type.h"
#include "common/types/log_record_type.h"

namespace YourSQL {

    class LogDiskManager;
    class BufferManager;
    class MetaPage;
    class TransactionManager;

    // 解析出的单条日志记录（内存视图）
    struct RecoveredLog {
        lsn_t lsn{};
        tx_id_t tx_id{};
        LogRecordType type{};
        std::vector<char> payload;
    };

    /**
     * ARIES 风格恢复：
     *   Analysis：扫描 WAL，重建每个事务的终态（提交/中止/未提交）与最大事务 id。
     *   Redo：按 LSN 顺序重放数据修改，对目标页做幂等应用（page.lsn < record.lsn 才应用）。
     *   Undo：不做物理 undo —— 未提交事务的版本因其 trx_id 不在已提交集合中，
     *          被 MVCC 可见性判断自然过滤。
     *
     * 当前执行器对数据页采用近似 force 策略（每次写后 Flush），
     * 故 Redo 主要负责 LSN 推进与状态校验；核心价值是重建事务终态供可见性使用。
     */
    class RecoveryManager {
    public:
        RecoveryManager(std::shared_ptr<LogDiskManager> log_disk,
                        std::shared_ptr<BufferManager> buffer,
                        std::shared_ptr<MetaPage> meta,
                        std::shared_ptr<TransactionManager> txn_mgr)
            : log_disk_(std::move(log_disk)), buffer_(std::move(buffer)),
              meta_(std::move(meta)), txn_mgr_(std::move(txn_mgr)) {}

        // 跑完整恢复流程，返回重放的日志条数
        auto Recover() -> size_t;

        // 仅解析 WAL（测试/调试用）
        auto ReadLog() -> std::vector<RecoveredLog>;

    private:
        auto Analysis(const std::vector<RecoveredLog> &logs) -> void;
        auto Redo(const std::vector<RecoveredLog> &logs) -> void;

        std::shared_ptr<LogDiskManager> log_disk_;
        std::shared_ptr<BufferManager> buffer_;
        std::shared_ptr<MetaPage> meta_;
        std::shared_ptr<TransactionManager> txn_mgr_;
    };

}
