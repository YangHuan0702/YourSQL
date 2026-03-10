//
// Created by huan.yang on 2026-03-02.
//
#pragma once

namespace YourSQL {
    enum class PhysicalOperatorTypes {
        PHYSICAL_SEQ_SCAN,
        PHYSICAL_FILTER,
        PHYSICAL_PROJECTION,
        PHYSICAL_LIMIT,
        PHYSICAL_HASH_AGGREGATION,
        PHYSICAL_HASH_JOIN,
        PHYSICAL_VALUES,
        PHYSICAL_INSERT,
    };
}
