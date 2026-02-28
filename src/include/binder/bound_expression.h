//
// Created by 杨欢 on 2026/2/14.
//

#pragma once
#include "common/types/column_types.h"
#include <vector>

namespace YourSQL {

    class BoundExpression {
    public:
        explicit BoundExpression(ColumnTypes types);
        virtual ~BoundExpression() = default;

        virtual auto to_string() -> std::string = 0;

        ColumnTypes return_type_;
        std::vector<std::unique_ptr<BoundExpression>> children_;

    };

}