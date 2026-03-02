//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include <memory>

#include "planner/logical/logical_operator.h"

namespace YourSQL {

    class Optimizer {
    public:
        explicit Optimizer(std::unique_ptr<LogicalOperator> logical_plan) : logical_plan_(std::move(logical_plan)) {}
        ~Optimizer() {}

        auto OptimizerLogicalPlan() -> std::unique_ptr<LogicalOperator> ;
    private:

        // 节点优化
        auto OptimizerNode(std::unique_ptr<LogicalOperator> node) -> std::unique_ptr<LogicalOperator>;

        // 谓词下推
        auto PushDownFilter(std::unique_ptr<LogicalOperator> node) -> std::unique_ptr<LogicalOperator>;

        // 常量折叠
        auto ConstantFold(std::unique_ptr<LogicalOperator> node) -> std::unique_ptr<LogicalOperator>;

        std::unique_ptr<LogicalOperator> logical_plan_;

    };


}
