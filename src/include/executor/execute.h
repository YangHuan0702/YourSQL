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
        explicit Execute(std::shared_ptr<ExecutorContext> context) : context_(context) {}
        ~Execute() = default;

        auto ExecuteQuery(std::unique_ptr<Executor> root) -> void;
        auto ExecuteInsert(std::unique_ptr<Executor> root) -> void;
        auto PrintTuple(const Tuple &tuple) -> void;

        std::shared_ptr<ExecutorContext> context_;
        bool header_printed_{false};
        Schema current_schema_;
        size_t row_count_{0};
    };

}
