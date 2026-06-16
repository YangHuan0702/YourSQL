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

        int capacity_{0};          // 单个缓冲区容量
        char *log_buffer_;
        char *flush_buffer_;
        int cur_offset_{0};        // 当前写缓冲已用字节
        int flush_size_{};         // 待刷缓冲的有效字节
        std::atomic<bool> is_flush_{false};
        std::atomic<bool> stop_{false};   // 通知后端线程退出
        std::mutex mutex_;
        std::condition_variable cv_backend_thread_;
        std::condition_variable cv_flush_thread_;
        std::unique_ptr<LogDiskManager> disk_manager_;
        std::thread flush_thread_;
    };

}
