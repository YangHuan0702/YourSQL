//
// Created by huan.yang on 2026-03-02.
//
#pragma once
#include "physical_operator.h"
#include "common/type.h"

namespace YourSQL {


    class PhysicalSeqScan : public PhysicalOperator {
    public:
        explicit PhysicalSeqScan(entry_id table_id) : PhysicalOperator(PhysicalOperatorTypes::PHYSICAL_SEQ_SCAN),table_id_(table_id),cursor_(0){}
        ~PhysicalSeqScan() override = default;

        auto to_string() -> std::string override;

        auto Open() -> void override;

        auto Next() -> bool override;

        auto Close() -> void override;

        entry_id table_id_;
        uint64_t cursor_;

    };

}
