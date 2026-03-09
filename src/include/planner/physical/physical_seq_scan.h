//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "physical_operator.h"
#include "common/type.h"

namespace YourSQL {

    class PhysicalSeqScan : public PhysicalOperator {
    public:
        explicit PhysicalSeqScan(entry_id table_id) : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN),table_id_(table_id){}
        ~PhysicalSeqScan() override = default;

        auto to_string() -> std::string override {
            return "SeqScan[TableId:" + std::to_string(table_id_) + "]";
        }

        entry_id table_id_;
    };

}
