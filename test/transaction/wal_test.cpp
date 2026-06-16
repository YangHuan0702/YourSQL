//
// 批次3 验证：WAL 写入 + 读回解析 + LogBuffer 行为
//
#include <cstring>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"

#include "common/type.h"
#include "common/types/log_record_type.h"
#include "log/log_buffer.h"
#include "log/log_manager.h"
#include "log/log_record.h"
#include "log/log_payload.h"
#include "storage/log_disk_manager.h"

using namespace YourSQL;

namespace {

auto TmpWalPath(const std::string &tag) -> std::string {
    // 用 tag 区分不同测试，避免共享文件
    return std::string("./test_wal_") + tag + ".log";
}

// 解析整个 WAL buffer，返回 (lsn, tx_id, type, payload) 列表
struct ParsedRecord {
    lsn_t lsn;
    tx_id_t tx_id;
    LogRecordType type;
    std::vector<char> payload;
};

auto ParseWal(const std::vector<char> &buf) -> std::vector<ParsedRecord> {
    std::vector<ParsedRecord> out;
    size_t off = 0;
    const size_t hdr = sizeof(lsn_t) + sizeof(tx_id_t) + sizeof(LogRecordType) + sizeof(uint32_t);
    while (off + hdr <= buf.size()) {
        ParsedRecord r;
        size_t p = off;
        memcpy(&r.lsn, buf.data() + p, sizeof(lsn_t));               p += sizeof(lsn_t);
        memcpy(&r.tx_id, buf.data() + p, sizeof(tx_id_t));           p += sizeof(tx_id_t);
        memcpy(&r.type, buf.data() + p, sizeof(LogRecordType));      p += sizeof(LogRecordType);
        uint32_t psize = 0;
        memcpy(&psize, buf.data() + p, sizeof(uint32_t));            p += sizeof(uint32_t);
        if (p + psize > buf.size()) break;
        r.payload.assign(buf.data() + p, buf.data() + p + psize);
        p += psize;
        out.push_back(std::move(r));
        off = p;
    }
    return out;
}

auto MakeDataRecord(tx_id_t txid, LogRecordType type, entry_id table_id, RID rid,
                    const std::string &image) -> LogRecord {
    LogDataPayload dp;
    dp.table_id = table_id;
    dp.rid = rid;
    dp.image.assign(image.begin(), image.end());
    auto enc = dp.Encode();
    char *payload = new char[enc.size()];
    memcpy(payload, enc.data(), enc.size());
    LogRecordHeader h{};
    h.tx_id_ = txid;
    h.type_ = type;
    h.payload_size_ = static_cast<uint32_t>(enc.size());
    return LogRecord(h, payload);
}

}  // namespace

TEST(TxnBatch3, WalWriteAndReadback) {
    std::string path = TmpWalPath("rw");
    std::filesystem::remove(path);

    {
        auto disk = std::make_unique<LogDiskManager>(path);
        auto buffer = std::make_unique<LogBuffer>(8192, std::move(disk));
        LogManager log(std::move(buffer));

        auto r1 = MakeDataRecord(1, LogRecordType::INSERT_ROW, 7, RID{2, 1}, "hello");
        auto r2 = MakeDataRecord(1, LogRecordType::DELETE_ROW, 7, RID{2, 1}, "");
        log.AppendLogRecord(r1);
        log.AppendLogRecord(r2);
        log.Flush();
    }  // LogBuffer 析构 join 后端线程

    LogDiskManager reader(path);
    auto buf = reader.ReadAll();
    auto records = ParseWal(buf);

    ASSERT_EQ(records.size(), 2u);
    EXPECT_EQ(records[0].type, LogRecordType::INSERT_ROW);
    EXPECT_EQ(records[1].type, LogRecordType::DELETE_ROW);
    EXPECT_EQ(records[0].lsn, 1u);
    EXPECT_EQ(records[1].lsn, 2u);

    auto p0 = LogDataPayload::Decode(records[0].payload.data(),
                                     static_cast<uint32_t>(records[0].payload.size()));
    EXPECT_EQ(p0.table_id, 7u);
    EXPECT_EQ(p0.rid.page_id_, 2u);
    EXPECT_EQ(p0.rid.row_id_, 1u);
    EXPECT_EQ(std::string(p0.image.begin(), p0.image.end()), "hello");

    std::filesystem::remove(path);
}

// 超长（超过缓冲容量）记录应被拒绝；多次写不丢数据
TEST(TxnBatch3, LogBufferMultiWriteAndFlush) {
    std::string path = TmpWalPath("multi");
    std::filesystem::remove(path);

    {
        auto disk = std::make_unique<LogDiskManager>(path);
        // 故意用小缓冲，触发自动刷盘换页
        auto buffer = std::make_unique<LogBuffer>(64, std::move(disk));
        LogManager log(std::move(buffer));

        // 每条记录 = 头 + payload；写多条触发缓冲换页
        for (int i = 0; i < 20; ++i) {
            auto r = MakeDataRecord(1, LogRecordType::INSERT_ROW, 1, RID{1, (row_id_t)(i + 1)}, "ab");
            log.AppendLogRecord(r);
        }
        log.Flush();
    }

    LogDiskManager reader(path);
    auto records = ParseWal(reader.ReadAll());
    EXPECT_EQ(records.size(), 20u);
    for (size_t i = 0; i < records.size(); ++i) {
        EXPECT_EQ(records[i].type, LogRecordType::INSERT_ROW);
    }

    std::filesystem::remove(path);
}
