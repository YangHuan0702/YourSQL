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
        uint32_t tuple_size_;
    };


    class Tuple {
    public:
        explicit Tuple(char *data, const Schema &schema) : data_(data), tuple_size_(schema.tuple_size_) {}

        char *data_;
        uint32_t tuple_size_;

    };

}
