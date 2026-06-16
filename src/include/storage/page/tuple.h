//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <cstring>
#include <vector>

#include "catalog/column_entry.h"
#include "transaction/undo_log.h"

namespace YourSQL {
    class Schema {
    public:
        std::vector<ColumnEntry> columns_;
        uint16_t tuple_size_;
    };


#define RECORD_DEL (1 << 0)
#define RECORD_DEAD (1 << 1)

    struct RecordHeader {
        // 最后一个插入或更新该记录的事务 ID
        tx_id_t trx_id_{};
        // 指向最近一条 undo 记录
        UndoPointer roll_ptr_{};
        uint16_t flags_{};
    };

    /**
     * 记录头在磁盘/页内的序列化布局（紧凑、无结构体对齐填充）：
     *   [ trx_id(8) | roll_ptr.page_id(8) | roll_ptr.slot(4) | flags(2) ]
     * 之后紧跟 null 位图（每列 1 字节）与各列 payload。
     *
     * 注意：不要用 sizeof(RecordHeader) 作为偏移，它含对齐填充，与实际写入不一致。
     */
#define REC_TRX_OFFSET      (0)
#define REC_ROLLPTR_OFFSET  (REC_TRX_OFFSET + sizeof(tx_id_t))
#define REC_ROLLPTR_SIZE    (sizeof(page_id_t) + sizeof(uint32_t))
#define REC_FLAGS_OFFSET    (REC_ROLLPTR_OFFSET + REC_ROLLPTR_SIZE)
#define REC_HEADER_SIZE     (REC_FLAGS_OFFSET + sizeof(uint16_t))

// 兼容旧名：payload（含 null 位图）从记录头之后开始
#define PAYLOAD_OFFSET REC_HEADER_SIZE


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
