//
// Created by huan.yang on 2026-03-23.
//
// WAL 数据修改记录的 payload 编解码。
// INSERT_ROW / DELETE_ROW / UPDATE_ROW 统一使用：
//   [ table_id(8) | page_id(8) | row_id(8) | image_size(4) | image_bytes ]
// 其中 image_bytes 为该操作后记录的完整字节（after-image，redo 用）。
// DELETE_ROW 的 image 可为空（仅靠 page_id/row_id 定位重做删除标记）。
//
#pragma once
#include <cstring>
#include <vector>

#include "common/type.h"
#include "storage/r_id.h"

namespace YourSQL {

    struct LogDataPayload {
        entry_id table_id{};
        RID rid{};
        std::vector<char> image;

        // 打包为连续字节
        auto Encode() const -> std::vector<char> {
            uint32_t image_size = static_cast<uint32_t>(image.size());
            size_t total = sizeof(entry_id) + sizeof(page_id_t) + sizeof(row_id_t)
                           + sizeof(uint32_t) + image.size();
            std::vector<char> buf(total);
            size_t off = 0;
            memcpy(buf.data() + off, &table_id, sizeof(entry_id));            off += sizeof(entry_id);
            memcpy(buf.data() + off, &rid.page_id_, sizeof(page_id_t));       off += sizeof(page_id_t);
            memcpy(buf.data() + off, &rid.row_id_, sizeof(row_id_t));         off += sizeof(row_id_t);
            memcpy(buf.data() + off, &image_size, sizeof(uint32_t));          off += sizeof(uint32_t);
            if (image_size > 0) {
                memcpy(buf.data() + off, image.data(), image_size);
            }
            return buf;
        }

        // 从连续字节解包
        static auto Decode(const char *data, uint32_t size) -> LogDataPayload {
            LogDataPayload p;
            size_t off = 0;
            if (size < sizeof(entry_id) + sizeof(page_id_t) + sizeof(row_id_t) + sizeof(uint32_t)) {
                return p;
            }
            memcpy(&p.table_id, data + off, sizeof(entry_id));        off += sizeof(entry_id);
            memcpy(&p.rid.page_id_, data + off, sizeof(page_id_t));   off += sizeof(page_id_t);
            memcpy(&p.rid.row_id_, data + off, sizeof(row_id_t));     off += sizeof(row_id_t);
            uint32_t image_size = 0;
            memcpy(&image_size, data + off, sizeof(uint32_t));        off += sizeof(uint32_t);
            p.image.resize(image_size);
            if (image_size > 0) {
                memcpy(p.image.data(), data + off, image_size);
            }
            return p;
        }
    };

}
