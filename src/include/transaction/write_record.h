//
// Created by huan.yang on 2026-03-23.
//
#pragma once
#include "common/type.h"
#include "storage/r_id.h"
#include "transaction/undo_log.h"

namespace YourSQL {

    // 事务对某条记录做了哪种写操作（用于 Abort 时逆向撤销）
    enum class WriteType : uint8_t {
        INSERT,             // 插入新行 -> 回滚时标记 dead
        DELETE,             // 标记删除旧行 -> 回滚时恢复原记录头
        UPDATE_DELETE_OLD,  // update 的旧行(被标记删除) -> 回滚时恢复
        UPDATE_INSERT_NEW,  // update 的新行(新插入) -> 回滚时标记 dead
    };

    // 一条写操作的撤销信息
    struct WriteRecord {
        RID rid;                    // 目标记录位置
        WriteType type;             // 操作类型
        entry_id table_id{};        // 所属表（回滚时维护行数计数）
        tx_id_t prev_trx_id{};      // 写之前该记录头的 trx_id（用于 DELETE/UPDATE_DELETE_OLD 恢复）
        UndoPointer prev_roll_ptr{};// 写之前该记录头的 roll_ptr
        uint16_t prev_flags{};      // 写之前该记录头的 flags
    };

}
