//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <memory>
#include <shared_mutex>

#include "tuple.h"
#include "buffer/page.h"
#include "storage/r_id.h"

namespace YourSQL {

#define NUM_ROWS_OFFSET sizeof(uint16_t)
#define SLOT_SIZE (sizeof(uint16_t) * 2 + 1)
#define HEADER_SIZE (NUM_ROWS_OFFSET + sizeof(uint32_t) + sizeof(uint64_t) * 2)

    struct TableHeader {
        uint16_t version;
        uint32_t num_rows;
        uint64_t page_id;
        uint64_t next_page_id;
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

        auto GetCount() -> uint32_t;

        auto InsertTuple(const Tuple &tuple,RID *rid) -> bool;
        auto updateTuple(const Tuple &tuple,const RID &rid) -> void;
        auto DeleteTuple(const RID &rid) -> void;
        auto GetTuple(const RID &rid, Tuple *tuple);
        auto GetPage() const -> Page* {
            return page_;
        }

    private:

        Page *page_;
        TableHeader header_{};
        uint32_t free_size;
        std::mutex mutex_;
    };
}
