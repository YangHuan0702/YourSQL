//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "common/types/physical_types.h"
#include <string>
#include <memory>
#include <vector>

namespace YourSQL {

    class PhysicalOperator {
    public:
        explicit PhysicalOperator(PhysicalOperatorTypes type) : type_(type){}
        virtual ~PhysicalOperator() = default;

        virtual auto to_string() -> std::string = 0;

        PhysicalOperatorTypes type_;
        std::vector<std::unique_ptr<PhysicalOperator>> children_;
    };

}
