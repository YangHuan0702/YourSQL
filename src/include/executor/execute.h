//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include <memory>

#include "executor.h"
#include "executor_context.h"

namespace YourSQL {

    class Execute {
    public:
        explicit Execute(std::shared_ptr<ExecutorContext> context,std::shared_ptr<BufferManager> buffer_manager) : context_(context),buffer_manager_(buffer_manager) {}
        ~Execute() = default;

        void ExecuteQuery(std::unique_ptr<Executor> root);

        std::shared_ptr<ExecutorContext> context_;
        std::shared_ptr<BufferManager> buffer_manager_;
    };

}
