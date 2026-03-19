//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include "common/type.h"
#include "common/types/log_record_type.h"

namespace YourSQL {

    struct LogRecordHeader {
        lsn_t lsn_;
        tx_id_t tx_id_;
        LogRecordType type_;
        uint32_t payload_size_;
    };

#define RECORD_HEADER_SIZE sizeof(LogRecordHeader)

    /**
     * wal file format:
     *-------------------------------------------
     *| record | record | record | record | ... |
     *-------------------------------------------
     *
     * record format:
     * -------------------------------------------------------------
     * | lsn | tx_id | type | pay_size |        payload            |
     * -------------------------------------------------------------
     */
    class LogRecord {
    public:
        explicit LogRecord(const LogRecordHeader &header,char *payload) : header_(header), payload_(payload) {}
        ~LogRecord() {
            delete [] payload_;
        }

        auto GetLsn() const -> lsn_t {
            return header_.lsn_;
        }
        auto GetTxId() const -> tx_id_t {
            return header_.tx_id_;
        }

        auto GetType() const -> LogRecordType {
            return header_.type_;
        }

        auto GetPayloadSize() const -> uint32_t {
            return header_.payload_size_;
        }
        auto GetPayload() const -> char* {
            return payload_;
        }

        // auto Serialize() const -> char* ;
        // auto Deserialize(char *) -> void ;

        LogRecordHeader header_;
        char *payload_;
    };




}
