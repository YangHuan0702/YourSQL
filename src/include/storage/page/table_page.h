//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include "tuple.h"
#include "buffer/page.h"
#include "storage/r_id.h"

namespace YourSQL {

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
        explicit TablePage(Page *page) : page_(page){}
        ~TablePage() = default;

        auto GetCount() -> int;
        auto GetHeader() -> TableHeader;

        auto InsertTuple(const Tuple &tuple,const RID &rid) -> void;
        auto updateTuple(const Tuple &tuple,const RID &rid) -> void;
        auto DeleteTuple(const RID &rid) -> void;
        auto GetTuple(const RID &rid, Tuple *tuple);
    private:
        Page *page_;
    };
}
