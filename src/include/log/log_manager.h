//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <atomic>
#include <memory>

#include "log_buffer.h"
#include "log_record.h"
#include "common/type.h"

namespace YourSQL {
    class LogManager {
    public:
        explicit LogManager(std::unique_ptr<LogBuffer> log_buffer);

        ~LogManager() = default;

        auto AppendLogRecord(const LogRecord &log_record) -> lsn_t;

        // 强制把日志缓冲刷到磁盘（commit 时调用，force-log-at-commit）
        auto Flush() -> void;

        auto GetNextLsn() -> lsn_t;

    private:
        std::unique_ptr<LogBuffer> buffer_;
        std::atomic<lsn_t> next_lsn_{1};
    };
}
