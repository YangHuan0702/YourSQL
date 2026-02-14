//
// Created by 杨欢 on 2026/2/13.
//

#pragma once

#include "common/types/plan_operator_types.h"
#include <vector>
#include <memory>

namespace YourSQL {

    class LogicalOperator {
    public:
        explicit LogicalOperator(PlanOperatorType type) : type_(type) {}
        virtual ~LogicalOperator() = 0;

        virtual auto to_string() -> void = 0;

        PlanOperatorType type_;
        std::vector<std::unique_ptr<LogicalOperator>> children_;
    };

}