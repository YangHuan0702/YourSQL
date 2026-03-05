//
// Created by 杨欢 on 2026/2/14.
//

#pragma once
#include "common/types/column_types.h"
#include <vector>
#include <memory>

#include "common/types/expression_types.h"


namespace YourSQL {

    class BoundExpression {
    public:
        explicit BoundExpression(ColumnTypes types,ExpressionType expr_type) : expression_type_(expr_type),return_type_(types) {}
        virtual ~BoundExpression() = default;

        virtual auto to_string() -> std::string = 0;

        auto AddChildren(std::unique_ptr<BoundExpression> children) -> void {
            children_.push_back(std::move(children));
        }

        ExpressionType expression_type_;
        ColumnTypes return_type_;
        std::vector<std::unique_ptr<BoundExpression>> children_;
    };

}
