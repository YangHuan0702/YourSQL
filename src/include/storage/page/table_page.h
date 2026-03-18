//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <memory>
#include <shared_mutex>

#include "tuple.h"
#include "buffer/meta_page.h"
#include "buffer/page.h"
#include "storage/r_id.h"
#include "transaction/transaction.h"

namespace YourSQL {
#define NUM_ROWS_OFFSET sizeof(uint16_t)
#define SLOT_SIZE sizeof(Slot)
#define HEADER_SIZE (NUM_ROWS_OFFSET + sizeof(uint32_t) + sizeof(page_id_t) * 2)


    struct TableHeader {
        uint16_t version;
        uint32_t num_rows;
        page_id_t page_id;
        page_id_t next_page_id;
    };

    struct Slot {
        uint16_t offset;
        uint16_t size;
        tx_id_t xmin_;
        tx_id_t xmax_;
        RID rid_;
        int16_t info_mask;
    };

    /**
     * format:
     * ------------------------------------------------------
     * | header | tuple-1 | tuple-2 | tuple-3 | ... | slots |
     * ------------------------------------------------------
     *
     * header:
     * -----------------------------------------------
     * | version | num_rows | page_id | next_page_id |
     * -----------------------------------------------
     *
     * slot:
     * --------------------------------------------------------------------
     * | tuple_offset | size | xmin | xmax | [page_id,row_id] | info_mask |
     * -------------------------------------------------------------------
     */

#define XMIN_OFFSET (sizeof(uint16_t) * 2)
#define XMAX_OFFSET (sizeof(uint16_t) * 2 + sizeof(tx_id_t))
#define INFO_MASK_OFFSET (SLOT_SIZE - sizeof(int16_t))
#define NEW_RID_OFFSET (XMAX_OFFSET + sizeof(tx_id_t))

    class TablePage {
    public:
        explicit TablePage(std::shared_ptr<MetaPage> meta_page, entry_id table_id, Page *page, bool read);

        ~TablePage() = default;

        auto GetCount(const Transaction&) const -> uint32_t;

        auto InsertTuple(const Tuple &tuple, RID *rid,const Transaction&) -> bool;

        auto updateTuple(const Tuple &tuple, const RID &rid,const Transaction&) -> void;

        auto updateTuple(const RID &rid, const RID &new_rid, Transaction &transaction) ->void;

        auto DeleteTuple(const RID &rid,const Transaction&) -> void;

        auto GetTuple(const RID &rid, Tuple *tuple,const Transaction&) -> void;

        auto SetXMax(const RID &rid, tx_id_t xmax) -> void;

        auto GetPage() const -> Page * {
            return page_;
        }

    private:
        std::shared_ptr<MetaPage> meta_page_;
        entry_id table_id_;
        Page *page_;
        TableHeader header_{};
        uint32_t free_size;
        std::mutex mutex_;
    };
}
