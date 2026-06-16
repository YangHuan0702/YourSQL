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

namespace YourSQL {

#define NUM_ROWS_OFFSET sizeof(uint16_t)
#define SLOT_SIZE (sizeof(uint16_t) * 2)
#define HEADER_SIZE sizeof(TableHeader)


    struct TableHeader {
        uint16_t version;
        uint32_t num_rows;
        page_id_t page_id;
        page_id_t next_page_id;
        lsn_t lsn_;
    };

    struct Slot {
        uint16_t offset;
        uint16_t size;
    };

    /**
     * format:
     * ------------------------------------------------------
     * | header | tuple-1 | tuple-2 | tuple-3 | ... | slots |
     * ------------------------------------------------------
     *
     * header:
     * -----------------------------------------------------
     * | version | num_rows | page_id | next_page_id | lsn |
     * -----------------------------------------------------
     *
     * slot:
     * -----------------------
     * | tuple_offset | size |
     * ----------------------
     *
     */
    class TablePage {
    public:
        explicit TablePage(std::shared_ptr<MetaPage> meta_page,entry_id table_id,Page *page,bool read);
        ~TablePage() = default;

        auto GetCount() const -> uint32_t;

        auto InsertTuple(const Tuple &tuple,RID *rid) -> bool;
        auto updateTuple(const Tuple &tuple,const RID &rid) -> void;
        auto DeleteTuple(const RID &rid) -> void;
        auto GetTuple(const RID &rid, Tuple *tuple) -> void;

        // MVCC：标记删除（不物理移除），写入删除事务 id 与旧版本 undo 指针
        auto MarkDelete(const RID &rid, tx_id_t trx_id, UndoPointer roll_ptr) -> void;

        // 回滚支持：把记录头恢复成给定的 trx_id / roll_ptr / flags（撤销删除标记）
        auto RestoreRecord(const RID &rid, tx_id_t trx_id, UndoPointer roll_ptr, uint16_t flags) -> void;
        // 回滚支持：把记录置为 dead（对所有快照不可见）
        auto MarkDead(const RID &rid) -> void;

        // 读取某条记录头中的 trx_id / flags / roll_ptr（不解析 payload）
        auto ReadRecordTrxId(const RID &rid) -> tx_id_t;
        auto ReadRecordFlags(const RID &rid) -> uint16_t;
        auto ReadRecordRollPtr(const RID &rid) -> UndoPointer;

        // 当前页是否还能容纳给定大小的 tuple（含 slot）
        auto HasSpaceFor(uint16_t tuple_size) const -> bool;

        // next_page_id 持久化访问
        auto GetNextPageId() const -> page_id_t { return header_.next_page_id; }
        auto SetNextPageId(page_id_t next_page_id) -> void;

        // 设置页 LSN（WAL：页修改后记录产生该修改的日志 lsn）
        auto SetLsn(lsn_t lsn) -> void;
        auto GetLsn() const -> lsn_t { return header_.lsn_; }

        [[nodiscard]] auto GetPage() const -> Page* {
            return page_;
        }

    private:
        // 把内存中的 header_ 写回页缓冲区
        auto WriteHeader() -> void;
        // 计算第 row_id 个 slot 在页内的起始偏移
        auto SlotOffset(row_id_t row_id) const -> size_t;

        std::shared_ptr<MetaPage> meta_page_;
        entry_id table_id_;
        Page *page_;
        TableHeader header_{};
        uint32_t free_size;
        std::mutex mutex_;
    };
}
