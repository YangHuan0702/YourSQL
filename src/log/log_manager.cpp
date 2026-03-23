//
// Created by huan.yang on 2026-03-19.
//
#include "log/log_manager.h"

#include <cstring>

using namespace YourSQL;

LogManager::LogManager(std::unique_ptr<LogBuffer> log_buffer) : buffer_(std::move(log_buffer)) {
}

auto LogManager::AppendLogRecord(const LogRecord &log_record) -> lsn_t {
    int arr_size = RECORD_HEADER_SIZE + log_record.header_.payload_size_;
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
    memcpy(record.get() + offset, log_record.payload_, log_record.header_.payload_size_);

    buffer_->Write(record.get(),arr_size);
    return id;
}

auto LogManager::GetNextLsn() -> lsn_t {
    return next_lsn_.fetch_add(1);
}
