//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "physical_operator.h"

namespace YourSQL {

    class PhysicalFilter : public PhysicalOperator {
    public:
        explicit PhysicalFilter() : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_FILTER){}

    };

}
