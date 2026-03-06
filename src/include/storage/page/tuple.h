//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <vector>

#include "catalog/column_entry.h"

namespace YourSQL {
    class Schema {
    public:
        std::vector<ColumnEntry> columns_;
        uint16_t tuple_size_;
    };


    class Tuple {
    public:
        Tuple() = default;
        explicit Tuple(char *data, const Schema &schema) : schema_(schema),data_(data), tuple_size_(schema.tuple_size_) {
        }
        ~Tuple() = default;

        Schema schema_;
        char *data_;
        uint16_t tuple_size_;
    };
}
