//
// Created by huan.yang on 2026-03-02.
//
#include "planner/planner.h"

#include "binder/bound_binary_expression.h"
#include "binder/bound_column_ref_expression.h"
#include "binder/bound_unary_expression.h"
#include "binder/bound_const_expression.h"
#include "planner/physical/physical_filter.h"
#include "planner/physical/physical_projection.h"
#include "planner/physical/physical_seq_scan.h"
#include "planner/physical/expression/physical_binary_expression.h"
#include "planner/physical/expression/physical_column_expression.h"
#include "planner/physical/expression/physical_constant_expression.h"
#include "planner/physical/expression/physical_unary_expression.h"

using namespace YourSQL;

auto Planner::CreateLogicalPlan(std::unique_ptr<BoundStatement> statement) -> std::unique_ptr<LogicalOperator> {
    switch (statement->type_) {
        case StatementType::SELECT: return LogicalSelectPlan(
                std::unique_ptr<BoundSelectStatement>(dynamic_cast<BoundSelectStatement *>(statement.release())));
        default: throw std::runtime_error("[LogicalPlanner] unknow statement type.");
    }
}

auto Planner::CreatePhysicalPlan(
    std::unique_ptr<LogicalOperator> logical_operator) -> std::unique_ptr<PhysicalOperator> {
    switch (logical_operator->type_) {
        case LogicalOperatorType::LOGICAL_GET: {
            auto logical_op = dynamic_cast<LogicalSeqScan *>(logical_operator.get());
            auto r = std::make_unique<PhysicalSeqScan>(logical_op->table_id_);
            return r;
        }
        case LogicalOperatorType::LOGICAL_FILTER: {
            auto logical_op = dynamic_cast<LogicalFilter *>(logical_operator.get());
            auto r = std::make_unique<PhysicalFilter>();
            for (auto &bound_expression : logical_op->expressions_) {
                r->expressions_.push_back(TransformExpression(bound_expression.get()));
            }
            return r;
        }
        case LogicalOperatorType::LOGICAL_PROJECTION : {
            auto logical_op = dynamic_cast<LogicalProjection *>(logical_operator.get());
            auto r = std::make_unique<PhysicalProjection>();
            for (auto &bound_expression : logical_op->expressions_) {
                r->columns_.push_back(TransformPhyColumnRefExpr(dynamic_cast<BoundColumnRefExpression *>(bound_expression.get())));
            }
            return r;
        }
        default: throw std::runtime_error("[LogicalPlanner] unknow logical operator type.");
    }
}

auto Planner::TransformPhyBinaryExpr(BoundBinaryExpression *comp_expression) -> std::unique_ptr<PhysicalExpression> {
    auto left = TransformExpression(comp_expression->children_[0].get());
    auto right = TransformExpression(comp_expression->children_[1].get());
    return std::make_unique<PhysicalBinaryExpression>(comp_expression->binary_op_, std::move(left), std::move(right));
}

auto Planner::TransformPhyUnaryExpr(BoundUnaryExpression *comp_expression) -> std::unique_ptr<PhysicalExpression> {
    return std::make_unique<PhysicalUnaryExpression>(comp_expression->table_id_, comp_expression->column_id_);
}

auto Planner::TransformPhyColumnRefExpr(
    BoundColumnRefExpression *column_ref_expression) -> std::unique_ptr<PhysicalExpression> {
    return std::make_unique<PhysicalColumnExpression>(column_ref_expression->table_id_,
                                                      column_ref_expression->column_id_);
}

auto Planner::TransformPhyConstExpr(BoundConstExpression *const_expression) -> std::unique_ptr<PhysicalExpression> {
    auto phy_const_expr = std::make_unique<PhysicalConstantExpression>(const_expression->value_);
    return phy_const_expr;
}


auto Planner::TransformExpression(BoundExpression *bound_expression) -> std::unique_ptr<PhysicalExpression> {
    switch (bound_expression->expression_type_) {
        case ExpressionType::BINARY: return TransformPhyBinaryExpr(
                dynamic_cast<BoundBinaryExpression *>(bound_expression));
        case ExpressionType::UNARY: return
                    TransformPhyUnaryExpr(dynamic_cast<BoundUnaryExpression *>(bound_expression));
        case ExpressionType::COLUMN_REF: return TransformPhyColumnRefExpr(
                dynamic_cast<BoundColumnRefExpression *>(bound_expression));
        case ExpressionType::CONST: return
                    TransformPhyConstExpr(dynamic_cast<BoundConstExpression *>(bound_expression));
        default: throw std::runtime_error("[Planner::TransformExpression] unknow expression type.");
    }
}
