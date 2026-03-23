//
// Created by huan.yang on 2026-03-19.
//
#include "log/log_buffer.h"

#include <cstring>
#include <thread>

using namespace YourSQL;

auto LogBuffer::BackendThreadMain() -> void {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_backend_thread_.wait(lock,[&]() {
            return is_flush_.load();
        });
        lock.unlock();

        disk_manager_->Write(flush_buffer_,flush_size_);

        lock.lock();

        is_flush_.store(false);
        cv_flush_thread_.notify_all();
    }
}

auto LogBuffer::Flush() -> void {
    std::unique_lock<std::mutex> lock(mutex_);
    SwapBuffers();
    is_flush_.store(true);
    cv_flush_thread_.wait(lock,[&]{ return !is_flush_.load(); });
    lock.unlock();
}

auto LogBuffer::Write(const char *data, int size) -> void {
    std::unique_lock<std::mutex> lock(mutex_);
    int after_size = cur_offset_ + size;
    if (after_size <= flush_size_) {
        memcpy(log_buffer_ + cur_offset_, data, size);

        if (after_size == flush_size_) {
            lock.unlock();
            Flush();
        }
    } else {
        lock.unlock();
        Flush();

        lock.lock();
        memcpy(log_buffer_ + cur_offset_, data, size);
        lock.unlock();
    }
}


auto LogBuffer::SwapBuffers() -> void {
    std::swap(log_buffer_,flush_buffer_);
    flush_size_ = cur_offset_;

    cur_offset_ = 0;
}


LogBuffer::LogBuffer(int buffer_size,std::unique_ptr<LogDiskManager> disk_manager) {
    log_buffer_ = new char[buffer_size];
    flush_buffer_ = new char[buffer_size];

    flush_size_ = 0;
    is_flush_ = false;

    disk_manager_ = std::move(disk_manager);
    flush_thread_ = std::thread(&LogBuffer::BackendThreadMain,this);
}


LogBuffer::~LogBuffer() {
    delete [] log_buffer_;
    delete [] flush_buffer_;
}

