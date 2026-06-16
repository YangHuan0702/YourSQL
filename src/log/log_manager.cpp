//
// Created by huan.yang on 2026-03-19.
//
#include "log/log_manager.h"

#include <cstring>

using namespace YourSQL;

LogManager::LogManager(std::unique_ptr<LogBuffer> log_buffer) : buffer_(std::move(log_buffer)) {
}

auto LogManager::AppendLogRecord(const LogRecord &log_record) -> lsn_t {
    // 精确头大小（不含结构体对齐填充）：lsn + tx_id + type + payload_size
    const int header_bytes = sizeof(lsn_t) + sizeof(tx_id_t) + sizeof(LogRecordType) + sizeof(uint32_t);
    int arr_size = header_bytes + log_record.header_.payload_size_;
    auto record = std::make_unique<char[]>(arr_size);
    lsn_t id = GetNextLsn();

    size_t offset = 0;
    memcpy(record.get() + offset, &id, sizeof(lsn_t));
    offset += sizeof(lsn_t);
    memcpy(record.get() + offset, &log_record.header_.tx_id_, sizeof(tx_id_t));
    offset += sizeof(tx_id_t);
    memcpy(record.get() + offset, &log_record.header_.type_, sizeof(LogRecordType));
    offset += sizeof(LogRecordType);
    memcpy(record.get() + offset, &log_record.header_.payload_size_, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    if (log_record.header_.payload_size_ > 0 && log_record.payload_ != nullptr) {
        memcpy(record.get() + offset, log_record.payload_, log_record.header_.payload_size_);
    }

    buffer_->Write(record.get(), arr_size);
    return id;
}

auto LogManager::GetNextLsn() -> lsn_t {
    return next_lsn_.fetch_add(1);
}

auto LogManager::Flush() -> void {
    buffer_->Flush();
}
