//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include "storage/page/tuple.h"

namespace YourSQL {

    class PhysicalExpression {
    public:
        explicit PhysicalExpression() = default;
        virtual ~PhysicalExpression() = default;

        virtual auto Evaluate(const Tuple &tuple) const -> Value = 0;
    };
}
