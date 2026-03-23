//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "storage/log_disk_manager.h"

namespace YourSQL {

    class LogBuffer {
    public:
        explicit LogBuffer(int buffer_size,std::unique_ptr<LogDiskManager> disk_manager);
        ~LogBuffer();

        auto Write(const char *,int) -> void;
        auto Flush() -> void;

    private:
        auto SwapBuffers() -> void;
        auto BackendThreadMain() -> void;

        char *log_buffer_;
        char *flush_buffer_;
        int cur_offset_{0};
        int flush_size_{};
        std::atomic<bool> is_flush_{false};
        std::mutex mutex_;
        std::condition_variable cv_backend_thread_;
        std::condition_variable cv_flush_thread_;
        std::unique_ptr<LogDiskManager> disk_manager_;
        std::thread flush_thread_;
    };

}
