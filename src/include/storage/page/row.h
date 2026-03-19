//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include <utility>
#include <vector>

#include "tuple.h"
#include "common/types/column_types.h"

namespace YourSQL {

    struct RecordHeader {
        // 最后一个插入或更新该记录的事务 ID
        tx_id_t trx_id_;
        // 指向最近一条 undo 记录
        undo_id_t roll_ptr_;
        uint16_t flags_;
    };

#define PAYLOAD_OFFSET sizeof(RecordHeader)


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

        RecordHeader header_{};
        size_t use_size_{};
        Schema schema_;
        std::vector<Value> values_;
    };

}
