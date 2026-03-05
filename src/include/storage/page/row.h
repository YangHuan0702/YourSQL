//
// Created by huan.yang on 2026-03-04.
//
#pragma once
#include <vector>

#include "tuple.h"
#include "common/types/column_types.h"

namespace YourSQL {

    class Row {
    public:
        explicit Row(Schema &schema) : schema_(schema) {}
        explicit Row(Schema &schema, const std::vector<Value> &values) : schema_(schema),values_(values){}
        ~Row() =default;

        [[nodiscard]] auto Serialize() const -> char* ;
        auto Deserialize(const Tuple &tuple) -> void ;

        auto GetValue(size_t index) -> Value;
        auto GetValue(const std::string &column_name) -> Value;
        auto SetValue(size_t index,const Value &value) -> void;

        Schema schema_;
        std::vector<Value> values_;
    };

}
