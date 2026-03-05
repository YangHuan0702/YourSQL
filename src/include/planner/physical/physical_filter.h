//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "physical_operator.h"

namespace YourSQL {

    class PhysicalFilter : public PhysicalOperator {
    public:
        explicit PhysicalFilter() : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_FILTER){}
        ~PhysicalFilter() override = default;

        auto to_string() -> std::string override;

        auto Open() -> void override;

        auto Next() -> bool override;

        auto Close() -> void override;

        std::vector<std::unique_ptr<PhysicalExpression>> expressions_;

    };

}
