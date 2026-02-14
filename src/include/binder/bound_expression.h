//
// Created by 杨欢 on 2026/2/14.
//

#pragma once
#include "common/types/column_types.h"


namespace YourSQL {

    class BoundExpression {
    public:
        explicit BoundExpression(ColumnTypes types);

        ColumnTypes return_type;
    };

}