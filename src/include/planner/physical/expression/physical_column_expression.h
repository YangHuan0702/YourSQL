//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include "physical_expression.h"
#include "storage/page/row.h"

namespace YourSQL {
    class PhysicalColumnExpression : public PhysicalExpression {
    public:
        explicit PhysicalColumnExpression(entry_id table_id, entry_id column_id) : table_id_(table_id),
            column_id_(column_id) {
        }

        ~PhysicalColumnExpression() override = default;

        auto Evaluate(const Tuple &tuple) const -> Value override {
            auto schema = tuple.schema_;
            Row r(schema);

            r.Deserialize(tuple);

            return r.GetValue(column_id_);
        }


        entry_id table_id_;
        entry_id column_id_;
    };
}
