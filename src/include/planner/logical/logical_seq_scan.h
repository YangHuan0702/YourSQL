//
// Created by huan.yang on 2026-03-02.
//

#pragma once
#include "logical_operator.h"
#include "common/type.h"

namespace YourSQL {

    class LogicalSeqScan final : public LogicalOperator {
    public:
        explicit LogicalSeqScan(entry_id table_id) : LogicalOperator(LogicalOperatorType::LOGICAL_GET), table_id_(table_id) {}
        ~LogicalSeqScan() override = default;

        auto to_string() -> std::string override {
            return "Logical Plan Seq Scan: [" + std::to_string(table_id_) + "]";
        }
        entry_id table_id_;
    };

}
