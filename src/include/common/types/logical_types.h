//
// Created by huan.yang on 2026-03-02.
//
#pragma once

namespace YourSQL {

    enum class LogicalOperatorType {
        LOGICAL_GET,
        LOGICAL_FILTER,
        LOGICAL_PROJECTION,
        LOGICAL_AGGREGATE,
        LOGICAL_LIMIT,
        LOGICAL_ORDER,
        LOGICAL_JOIN,
        LOGICAL_INSERT,
        LOGICAL_DELETE,
        LOGICAL_UPDATE,
        LOGICAL_EXPLAIN
    };

}