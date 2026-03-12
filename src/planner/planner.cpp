//
// Created by huan.yang on 2026-03-02.
//
#include "planner/planner.h"

#include "binder/bound_binary_expression.h"
#include "binder/bound_column_ref_expression.h"
#include "binder/bound_unary_expression.h"
#include "binder/bound_const_expression.h"
#include "binder/statement/bound_insert_statement.h"
#include "planner/logical/logical_filter.h"
#include "planner/logical/logical_insert.h"
#include "planner/logical/logical_projection.h"
#include "planner/logical/logical_seq_scan.h"
#include "planner/logical/logical_values.h"
#include "planner/physical/physical_filter.h"
#include "planner/physical/physical_insert.h"
#include "planner/physical/physical_projection.h"
#include "planner/physical/physical_seq_scan.h"
#include "planner/physical/physical_values.h"
#include "planner/physical/expression/physical_binary_expression.h"
#include "planner/physical/expression/physical_column_expression.h"
#include "planner/physical/expression/physical_constant_expression.h"
#include "planner/physical/expression/physical_unary_expression.h"

using namespace YourSQL;

auto Planner::CreateLogicalPlan(std::unique_ptr<BoundStatement> statement) -> std::unique_ptr<LogicalOperator> {
    switch (statement->type_) {
        case StatementType::SELECT: return LogicalSelectPlan(
                std::unique_ptr<BoundSelectStatement>(dynamic_cast<BoundSelectStatement *>(statement.release())));
        case StatementType::INSERT: return LogicalInsertPlan(
                std::unique_ptr<BoundInsertStatement>(dynamic_cast<BoundInsertStatement *>(statement.release())));
        default: throw std::runtime_error("[LogicalPlanner] unknow statement type.");
    }
}

auto Planner::CreatePhysicalPlan(
    const std::unique_ptr<LogicalOperator> &logical_operator) -> std::unique_ptr<PhysicalOperator> {
    switch (logical_operator->type_) {
        case LogicalOperatorType::LOGICAL_INSERT: {
            auto logical_op = dynamic_cast<LogicalInsert*>(logical_operator.get());
            auto r = std::make_unique<PhysicalInsert>(logical_op->table_id_,logical_op->column_ids_);
            r->children_.push_back(CreatePhysicalPlan(logical_operator->children_[0]));
            return r;
        }
        case LogicalOperatorType::LOGICAL_VALUES: {
            auto logical_op = dynamic_cast<LogicalValues*>(logical_operator.get());
            return std::make_unique<PhysicalValues>(logical_op->column_ids_,logical_op->values_);
        }
        case LogicalOperatorType::LOGICAL_GET: {
            auto logical_op = dynamic_cast<LogicalSeqScan *>(logical_operator.get());
            auto r = std::make_unique<PhysicalSeqScan>(logical_op->table_id_);
            return r;
        }
        case LogicalOperatorType::LOGICAL_FILTER: {
            // auto logical_op = dynamic_cast<LogicalFilter *>(logical_operator.get());
            auto r = std::make_unique<PhysicalFilter>();
            // for (auto &bound_expression : logical_op->expressions_) {
            // r->expressions_.push_back(TransformExpression(bound_expression.get()));
            // }
            return r;
        }
        case LogicalOperatorType::LOGICAL_PROJECTION: {
            auto logical_op = dynamic_cast<LogicalProjection *>(logical_operator.get());
            auto r = std::make_unique<PhysicalProjection>();
            for (auto &bound_expression: logical_op->expressions_) {
                auto expr = dynamic_cast<BoundColumnRefExpression *>(bound_expression.get());
                r->columns_.push_back(expr->column_id_);
            }
            return r;
        }
        default: throw std::runtime_error("[LogicalPlanner] unknow logical operator type.");
    }
}

auto Planner::TransformPhyBinaryExpr(BoundBinaryExpression *comp_expression) -> std::unique_ptr<PhysicalExpression> {
    auto left = TransformExpression(comp_expression->children_[0]);
    auto right = TransformExpression(comp_expression->children_[1]);
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

auto Planner::TransformExpression(
    std::unique_ptr<BoundExpression> &bound_expression) -> std::unique_ptr<PhysicalExpression> {
    switch (bound_expression->expression_type_) {
        case ExpressionType::BINARY: return TransformPhyBinaryExpr(
                dynamic_cast<BoundBinaryExpression *>(bound_expression.release()));
        case ExpressionType::UNARY: return
                    TransformPhyUnaryExpr(dynamic_cast<BoundUnaryExpression *>(bound_expression.release()));
        case ExpressionType::COLUMN_REF: return TransformPhyColumnRefExpr(
                dynamic_cast<BoundColumnRefExpression *>(bound_expression.release()));
        case ExpressionType::CONST: return
                    TransformPhyConstExpr(dynamic_cast<BoundConstExpression *>(bound_expression.release()));
        default: throw std::runtime_error("[Planner::TransformExpression] unknow expression type.");
    }
}

auto Planner::TransformExpression(
    std::vector<std::unique_ptr<BoundExpression> > &expressions) -> std::unique_ptr<PhysicalExpression> {
    std::vector<std::unique_ptr<PhysicalExpression> > expr_arr;
    expr_arr.reserve(expressions.size());
    for (auto &bound_expression: expressions) {
        expr_arr.push_back(TransformExpression(bound_expression));
    }
    if (expr_arr.empty()) return nullptr;
    if (expr_arr.size() == 1) return std::move(expr_arr[0]);

    auto result = std::move(expr_arr[0]);
    for (size_t index = 1; index < expr_arr.size(); ++index) {
        result = std::make_unique<PhysicalBinaryExpression>(BinaryOp::AND, std::move(result),
                                                            std::move(expr_arr[index]));
    }
    return result;
}
