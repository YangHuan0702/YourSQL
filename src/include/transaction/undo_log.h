//
// Created by huan.yang on 2026-03-18.
//

#pragma once
#include <cstring>
#include <vector>

#include "buffer/page.h"
#include "common/type.h"

namespace YourSQL {
    enum class UndoType : uint8_t {
        INSERT,
        UPDATE,
        DELETE
    };

    struct UndoPointer {
        page_id_t page_id_;
        uint32_t slot;

        auto IsNull() const -> bool { return page_id_ == 0; }
    };

    // 内存中的 undo 记录；payload 用独立缓冲，避免柔性数组带来的悬空/越界
    struct UndoLogRecord {
        UndoPointer old_roll_ptr_{};
        tx_id_t old_trx_id_{};
        uint32_t payload_size_{};
        std::vector<char> payload_{};
    };

    /**
     * undo-log page format:
     * -----------------------------------------------------------------
     * | page_id | lsn_ | free_offset | record | record | record | ... |
     * -----------------------------------------------------------------
     *
     * undo-log record format :
     * --------------------------------------------------------------------
     * | old_roll_ptr(page_id,slot) | old_trx_id | payload_size | payload |
     * --------------------------------------------------------------------
     */
    class UndoLogPage {
    public:
        explicit UndoLogPage(Page *page) : page_(page) {}
        ~UndoLogPage()  = default;

        // 初始化一个全新的 undo 页（写入页头）
        auto InitNew(page_id_t page_id) -> void;
        // 从已有页缓冲解析页头到成员
        auto Load() -> void;

        // 追加一条记录，返回该记录在页内的起始偏移；空间不足返回 false
        auto AppendRecord(const UndoPointer &old_roll_ptr, tx_id_t old_trx_id,
                          const char *payload, uint32_t payload_size, uint32_t *out_offset) -> bool;
        auto ReadRecord(uint32_t offset, UndoLogRecord *record) const -> void;

        auto GetFreeOffset() const -> uint32_t { return free_offset_; }
        auto HasSpaceFor(uint32_t payload_size) const -> bool;

        page_id_t page_id_{};
        lsn_t lsn_{};
        uint32_t free_offset_{};
        Page *page_;

    private:
        // 页头大小：page_id + lsn + free_offset
        static constexpr uint32_t kHeaderSize =
            sizeof(page_id_t) + sizeof(lsn_t) + sizeof(uint32_t);
        auto WriteHeader() -> void;
    };


}
