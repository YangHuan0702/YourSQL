//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include "tuple.h"
#include "buffer/page.h"
#include "storage/r_id.h"

namespace YourSQL {

#define NUM_ROWS_OFFSET sizeof(uint16_t)
#define SLOT_SIZE (sizeof(uint16_t) * 2 + 1)
#define HEADER_SIZE (NUM_ROWS_OFFSET + sizeof(uint32_t))

    struct TableHeader {
        uint16_t version;
        uint32_t num_rows;
    };

    struct Slot {
        uint16_t offset;
        uint16_t size;
        bool deleted;
    };


    class TablePage {
    public:
        explicit TablePage(Page *page);
        ~TablePage() = default;

        auto GetCount() -> int;

        auto InsertTuple(const Tuple &tuple,const RID &rid) -> void;
        auto updateTuple(const Tuple &tuple,const RID &rid) -> void;
        auto DeleteTuple(const RID &rid) -> void;
        auto GetTuple(const RID &rid, Tuple *tuple);


    private:
        Page *page_;
        TableHeader header_{};
        uint32_t free_size;
    };
}
