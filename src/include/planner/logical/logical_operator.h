//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "common/types/logical_types.h"
#include <vector>
#include <memory>

namespace YourSQL {

    class LogicalOperator {
    public:
        explicit LogicalOperator(LogicalOperatorType type) : type_(type) {}
        virtual ~LogicalOperator() = default;

        virtual auto to_string () -> std::string = 0;

        LogicalOperatorType type_;
        std::vector<std::unique_ptr<LogicalOperator>> children_;
    };

}
