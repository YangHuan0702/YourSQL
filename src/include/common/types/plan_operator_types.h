//
// Created by 杨欢 on 2026/2/13.
//
#pragma once

namespace YourSQL {

    enum class PlanOperatorType {
        SCAN,
        FILTER,
        PROJECTION,
        JOIN,
        AGGREGATE,
        SORT,
        LIMIT
    };

}
