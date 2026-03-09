//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include <memory>

#include "binder/bound_binary_expression.h"
#include "binder/bound_column_ref_expression.h"
#include "binder/bound_unary_expression.h"
#include "binder/bound_const_expression.h"
#include "binder/statement/bound_select_statement.h"
#include "logical/logical_operator.h"
#include "physical/physical_operator.h"
#include "physical/expression/physical_expression.h"

/**
 * TODO(设计 Vectorized 执行模型)
 */
namespace YourSQL {
    class Planner {
    public:
        explicit Planner() = default;

        ~Planner() = default;

        auto CreateLogicalPlan(std::unique_ptr<BoundStatement> statement) -> std::unique_ptr<LogicalOperator>;

        auto CreatePhysicalPlan(const std::unique_ptr<LogicalOperator> &) -> std::unique_ptr<PhysicalOperator>;

    private:
        auto PhysicalTransformerGet(std::unique_ptr<LogicalOperator> &) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalTransformerFilter(std::unique_ptr<LogicalOperator> &) -> std::unique_ptr<PhysicalOperator>;
        auto PhysicalTransformerProjection(std::unique_ptr<LogicalOperator> &) -> std::unique_ptr<PhysicalOperator>;

        auto TransformPhyBinaryExpr(BoundBinaryExpression *) -> std::unique_ptr<PhysicalExpression>;
        static auto TransformPhyUnaryExpr(BoundUnaryExpression *) -> std::unique_ptr<PhysicalExpression>;
        static auto TransformPhyColumnRefExpr(BoundColumnRefExpression *) -> std::unique_ptr<PhysicalExpression>;
        static auto TransformPhyConstExpr(BoundConstExpression *) -> std::unique_ptr<PhysicalExpression>;

        auto TransformExpression(std::unique_ptr<BoundExpression> &) -> std::unique_ptr<PhysicalExpression>;
        auto TransformExpression(std::vector<std::unique_ptr<BoundExpression>> &) -> std::unique_ptr<PhysicalExpression>;

        auto LogicalSelectPlan(
            std::unique_ptr<BoundSelectStatement> select_statement) -> std::unique_ptr<LogicalOperator>;
    };
}
