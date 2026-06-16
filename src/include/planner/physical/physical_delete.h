//
// Created by huan.yang on 2026-03-22.
//
#pragma once
#include <memory>

#include "physical_operator.h"
#include "planner/physical/expression/physical_expression.h"
#include "common/type.h"

namespace YourSQL {
    class PhysicalDelete : public PhysicalOperator {
    public:
        explicit PhysicalDelete(entry_id table_id, std::unique_ptr<PhysicalExpression> filter)
            : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_DELETE),
              table_id_(table_id), filter_(std::move(filter)) {
        }

        ~PhysicalDelete() override = default;

        auto to_string() -> std::string override {
            return "PhysicalDelete";
        }

        entry_id table_id_;
        std::unique_ptr<PhysicalExpression> filter_;  // 可为空（无 WHERE）
    };
}
