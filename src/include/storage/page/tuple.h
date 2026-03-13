//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <cstring>
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

        explicit Tuple(char *data, const Schema &schema) : schema_(schema), data_(data),
                                                           tuple_size_(schema.tuple_size_) {
        }

        Tuple(const Tuple &other) : schema_(other.schema_), tuple_size_(other.tuple_size_),
                                    query_result_(other.query_result_) {
            if (other.data_ != nullptr && other.tuple_size_ > 0) {
                data_ = new char[tuple_size_];
                memcpy(data_, other.data_, tuple_size_);
            } else {
                data_ = nullptr;
            }
        }

        Tuple(Tuple &&other) noexcept : schema_(std::move(other.schema_)), data_(other.data_),
                                        tuple_size_(other.tuple_size_), query_result_(std::move(other.query_result_)) {
            other.data_ = nullptr;
            other.tuple_size_ = 0;
        }

        Tuple &operator=(const Tuple &other) {
            if (this != &other) {
                delete[] data_;

                schema_ = other.schema_;
                tuple_size_ = other.tuple_size_;
                query_result_ = other.query_result_;

                if (other.data_ != nullptr && other.tuple_size_ > 0) {
                    data_ = new char[tuple_size_];
                    memcpy(data_, other.data_, tuple_size_);
                } else {
                    data_ = nullptr;
                }
            }
            return *this;
        }

        Tuple &operator=(Tuple &&other) noexcept {
            if (this != &other) {
                delete[] data_;

                schema_ = std::move(other.schema_);
                data_ = other.data_;
                tuple_size_ = other.tuple_size_;
                query_result_ = std::move(other.query_result_);

                other.data_ = nullptr;
                other.tuple_size_ = 0;
            }
            return *this;
        }

        ~Tuple() = default;

        auto SetQueryResult(const std::vector<Value> &values) -> void {
            query_result_ = values;
        }

        auto Copy(const Tuple &tuple) -> void {
            if (this != &tuple) {
                delete[] data_;

                this->schema_ = tuple.schema_;
                this->tuple_size_ = tuple.tuple_size_;
                this->query_result_ = tuple.query_result_;

                if (tuple.data_ != nullptr && tuple.tuple_size_ > 0) {
                    data_ = new char[tuple_size_];
                    memcpy(data_, tuple.data_, tuple_size_);
                } else {
                    data_ = nullptr;
                }
            }
        }

        Schema schema_;
        char *data_{nullptr};
        uint16_t tuple_size_{0};
        std::vector<Value> query_result_{};
    };
}
