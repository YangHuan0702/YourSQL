//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "buffer/buffer_manager.h"

namespace YourSQL {
    class ExecutorContext {
    public:
        explicit ExecutorContext(const std::shared_ptr<BufferManager> &buffer_manager) : buffer_manager_(buffer_manager) {}
        ~ExecutorContext()  = default;


        std::shared_ptr<BufferManager> buffer_manager_;
    };
}
