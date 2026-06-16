//
// Created by huan.yang on 2026-03-19.
//
#include "log/log_buffer.h"

#include <cstring>
#include <stdexcept>
#include <thread>

using namespace YourSQL;

LogBuffer::LogBuffer(int buffer_size, std::unique_ptr<LogDiskManager> disk_manager) {
    capacity_ = buffer_size;
    log_buffer_ = new char[buffer_size];
    flush_buffer_ = new char[buffer_size];
    cur_offset_ = 0;
    flush_size_ = 0;
    is_flush_ = false;
    stop_ = false;
    disk_manager_ = std::move(disk_manager);
    flush_thread_ = std::thread(&LogBuffer::BackendThreadMain, this);
}

LogBuffer::~LogBuffer() {
    // 刷掉残留数据并通知后端线程退出
    Flush();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_.store(true);
    }
    cv_backend_thread_.notify_all();
    if (flush_thread_.joinable()) {
        flush_thread_.join();
    }
    delete[] log_buffer_;
    delete[] flush_buffer_;
}

auto LogBuffer::BackendThreadMain() -> void {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_backend_thread_.wait(lock, [&]() {
            return is_flush_.load() || stop_.load();
        });

        if (is_flush_.load()) {
            int size = flush_size_;
            lock.unlock();
            if (size > 0) {
                disk_manager_->Write(flush_buffer_, size);
            }
            lock.lock();
            is_flush_.store(false);
            cv_flush_thread_.notify_all();
        }

        if (stop_.load() && !is_flush_.load()) {
            break;
        }
    }
}

auto LogBuffer::Flush() -> void {
    std::unique_lock<std::mutex> lock(mutex_);
    if (cur_offset_ == 0) {
        return;  // 无数据可刷
    }
    SwapBuffers();
    is_flush_.store(true);
    cv_backend_thread_.notify_all();
    cv_flush_thread_.wait(lock, [&] { return !is_flush_.load(); });
}

auto LogBuffer::Write(const char *data, int size) -> void {
    if (size > capacity_) {
        throw std::runtime_error("LogBuffer::Write record larger than buffer capacity");
    }
    std::unique_lock<std::mutex> lock(mutex_);
    // 当前缓冲放不下，先刷盘腾空
    if (cur_offset_ + size > capacity_) {
        SwapBuffers();
        is_flush_.store(true);
        cv_backend_thread_.notify_all();
        cv_flush_thread_.wait(lock, [&] { return !is_flush_.load(); });
    }
    memcpy(log_buffer_ + cur_offset_, data, size);
    cur_offset_ += size;
}

auto LogBuffer::SwapBuffers() -> void {
    std::swap(log_buffer_, flush_buffer_);
    flush_size_ = cur_offset_;
    cur_offset_ = 0;
}
