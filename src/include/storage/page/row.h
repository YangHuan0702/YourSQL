//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include <utility>
#include <vector>

#include "tuple.h"
#include "common/types/column_types.h"
#include "transaction/undo_log.h"

namespace YourSQL {

    class Row {
    public:
        explicit Row(Schema schema) : schema_(std::move(schema)) {}
        explicit Row(Schema schema, const std::vector<Value> &values) : schema_(std::move(schema)),values_(values){}
        ~Row() =default;

        [[nodiscard]] auto Serialize() -> char* ;
        auto Deserialize(const Tuple &tuple) -> void ;

        auto GetValue(size_t index) -> Value;
        auto GetValue(const std::string &column_name) -> Value;
        auto SetValue(size_t index,const Value &value) -> void;

        auto SetTrxId(tx_id_t tx_id) -> void {
            header_.trx_id_ = tx_id;
        }
        auto SetRollPtr(UndoPointer roll_ptr_) -> void {
            header_.roll_ptr_ = roll_ptr_;
        }
        auto SetFlags(uint16_t flag) -> void {
            header_.flags_ = flag;
        }
        auto IsDel() const -> bool {
            return header_.flags_ & RECORD_DEL;
        }

        RecordHeader header_{};
        size_t use_size_{};
        Schema schema_;
        std::vector<Value> values_;
    };

}
