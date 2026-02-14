//
// Created by 杨欢 on 2026/2/13.
//
#include "parser/expression/logic_expression.h"

#include "parser/transformer.h"

using namespace YourSQL;


auto Transformer::transformLogicExpr(hsql::Expr *expr) -> std::unique_ptr<BaseExpression> {
    switch (expr->opType) {
        case hsql::kOpNotEquals:
        case hsql::kOpEquals: {
            OperatorType type = expr->opType == hsql::kOpEquals ? OperatorType::EQ : OperatorType::NEQ;
            std::string column = expr->expr->getName();
            //TODO：这里需要根据字段的类型来获取指定位置的值
            std::string val = expr->expr2->getName();
            return std::make_unique<LogicExpression>(type,column,Value(val));
        }
        case hsql::kOpLess:
        case hsql::kOpLessEq:
        case hsql::kOpGreater:
        case hsql::kOpGreaterEq :{
            OperatorType logicType = OperatorType::INVALID;
            if (hsql::kOpLess == expr->opType) {
                logicType= OperatorType::LT;
            } else if (hsql::kOpLessEq == expr->opType) {
                logicType= OperatorType::LTE;
            } else if (hsql::kOpGreater == expr->opType) {
                logicType= OperatorType::GT;
            } else if (hsql::kOpGreaterEq == expr->opType) {
                logicType= OperatorType::GTE;
            }

            std::string c_name = expr->expr->getName();
            long long val = expr->expr2->ival;
            return std::make_unique<LogicExpression>(logicType,c_name,Value(val));
        }
        default:
            throw std::runtime_error("unknow the logical OpType");
    }
}
