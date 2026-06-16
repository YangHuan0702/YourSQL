//
// 批次4 验证：ARIES 恢复（Analysis 重建事务终态；未提交靠可见性过滤）
//
#include <cstring>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"

#include "common/type.h"
#include "common/types/log_record_type.h"
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "log/log_buffer.h"
#include "log/log_manager.h"
#include "log/log_record.h"
#include "log/log_payload.h"
#include "storage/log_disk_manager.h"
#include "storage/posix_disk_manager.h"
#include "transaction/recovery_manager.h"
#include "transaction/transaction_manager.h"

using namespace YourSQL;

namespace {

auto WalPath(const std::string &tag) -> std::string {
    return std::string("./test_recover_") + tag + ".log";
}

auto DataRecord(tx_id_t txid, LogRecordType type, entry_id table_id, RID rid) -> LogRecord {
    LogDataPayload dp;
    dp.table_id = table_id;
    dp.rid = rid;
    auto enc = dp.Encode();
    char *payload = new char[enc.size()];
    memcpy(payload, enc.data(), enc.size());
    LogRecordHeader h{};
    h.tx_id_ = txid;
    h.type_ = type;
    h.payload_size_ = static_cast<uint32_t>(enc.size());
    return LogRecord(h, payload);
}

auto CtrlRecord(tx_id_t txid, LogRecordType type) -> LogRecord {
    LogRecordHeader h{};
    h.tx_id_ = txid;
    h.type_ = type;
    h.payload_size_ = 0;
    return LogRecord(h, nullptr);
}

}  // namespace

// 已提交事务 -> committed；崩溃前未提交事务 -> aborted（可见性据此过滤）
TEST(TxnBatch4, RecoveryRebuildsTxnState) {
    std::string path = WalPath("state");
    std::filesystem::remove(path);

    // 写一段日志：txn1 提交，txn2 未提交（无 COMMIT）
    {
        auto disk = std::make_unique<LogDiskManager>(path);
        auto buffer = std::make_unique<LogBuffer>(8192, std::move(disk));
        LogManager log(std::move(buffer));

        auto a1 = CtrlRecord(1, LogRecordType::BEGIN);
        auto a2 = DataRecord(1, LogRecordType::INSERT_ROW, 5, RID{2, 1});
        auto a3 = CtrlRecord(1, LogRecordType::COMMIT);
        log.AppendLogRecord(a1);
        log.AppendLogRecord(a2);
        log.AppendLogRecord(a3);

        auto b1 = CtrlRecord(2, LogRecordType::BEGIN);
        auto b2 = DataRecord(2, LogRecordType::INSERT_ROW, 5, RID{2, 2});
        log.AppendLogRecord(b1);
        log.AppendLogRecord(b2);
        // txn2 无 COMMIT，模拟崩溃
        log.Flush();
    }

    // 跑恢复
    auto pdisk = std::make_shared<PosixDiskManager>();
    auto buffer = std::make_shared<BufferManager>(pdisk);
    auto meta = std::make_shared<MetaPage>(buffer);
    auto txn_mgr = std::make_shared<TransactionManager>();
    auto recover_disk = std::make_shared<LogDiskManager>(path);

    RecoveryManager recovery(recover_disk, buffer, meta, txn_mgr);
    size_t replayed = recovery.Recover();
    EXPECT_EQ(replayed, 5u);

    // txn1 已提交、txn2 视为中止
    EXPECT_TRUE(txn_mgr->IsCommitted(1));
    EXPECT_FALSE(txn_mgr->IsCommitted(2));
    EXPECT_TRUE(txn_mgr->IsAborted(2));

    // next_txid 推进到 max+1，新事务 id 不会与历史冲突
    auto fresh = txn_mgr->Begin();
    EXPECT_GT(fresh->tx_id_, 2u);

    std::filesystem::remove(path);
}

// 空日志：恢复是 no-op
TEST(TxnBatch4, RecoveryEmptyLog) {
    std::string path = WalPath("empty");
    std::filesystem::remove(path);
    { LogDiskManager d(path); }  // 创建空文件

    auto pdisk = std::make_shared<PosixDiskManager>();
    auto buffer = std::make_shared<BufferManager>(pdisk);
    auto meta = std::make_shared<MetaPage>(buffer);
    auto txn_mgr = std::make_shared<TransactionManager>();
    auto recover_disk = std::make_shared<LogDiskManager>(path);

    RecoveryManager recovery(recover_disk, buffer, meta, txn_mgr);
    EXPECT_EQ(recovery.Recover(), 0u);

    std::filesystem::remove(path);
}

// 重复恢复幂等：跑两次结果一致
TEST(TxnBatch4, RecoveryIdempotent) {
    std::string path = WalPath("idem");
    std::filesystem::remove(path);

    {
        auto disk = std::make_unique<LogDiskManager>(path);
        auto buffer = std::make_unique<LogBuffer>(8192, std::move(disk));
        LogManager log(std::move(buffer));
        auto a1 = DataRecord(1, LogRecordType::INSERT_ROW, 5, RID{2, 1});
        auto a2 = CtrlRecord(1, LogRecordType::COMMIT);
        log.AppendLogRecord(a1);
        log.AppendLogRecord(a2);
        log.Flush();
    }

    auto pdisk = std::make_shared<PosixDiskManager>();
    auto buffer = std::make_shared<BufferManager>(pdisk);
    auto meta = std::make_shared<MetaPage>(buffer);
    auto txn_mgr = std::make_shared<TransactionManager>();

    {
        auto rd = std::make_shared<LogDiskManager>(path);
        RecoveryManager r1(rd, buffer, meta, txn_mgr);
        r1.Recover();
    }
    {
        auto rd = std::make_shared<LogDiskManager>(path);
        RecoveryManager r2(rd, buffer, meta, txn_mgr);
        r2.Recover();
    }
    EXPECT_TRUE(txn_mgr->IsCommitted(1));

    std::filesystem::remove(path);
}
