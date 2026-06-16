//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "catalog/catalog.h"
#include "log/log_manager.h"
#include "log/log_payload.h"
#include "common/types/log_record_type.h"
#include "transaction/transaction_manager.h"
#include "transaction/undo_log_manager.h"

namespace YourSQL {
    class ExecutorContext {
    public:
        explicit ExecutorContext(const std::shared_ptr<Catalog> &catalog,
                                 const std::shared_ptr<BufferManager> &buffer_manager,
                                 const std::shared_ptr<MetaPage> &meta_page,
                                 const std::shared_ptr<TransactionManager> &transaction_manager,
                                 const std::shared_ptr<Transaction> &transaction,
                                 const std::shared_ptr<UndoLogManager> &undo_log_manager,
                                 const std::shared_ptr<LogManager> &log_manager = nullptr) : catalog_(catalog),
            buffer_manager_(buffer_manager),
            meta_page_(meta_page), transaction_manager_(transaction_manager), transaction_(transaction),
            undo_log_manager_(undo_log_manager), log_manager_(log_manager) {
        }

        ~ExecutorContext() = default;

        std::shared_ptr<Catalog> catalog_;

        std::shared_ptr<BufferManager> buffer_manager_;

        std::shared_ptr<MetaPage> meta_page_;


        std::shared_ptr<TransactionManager> transaction_manager_;
        std::shared_ptr<Transaction> transaction_;
        std::shared_ptr<UndoLogManager> undo_log_manager_;
        std::shared_ptr<LogManager> log_manager_;

        // 写一条数据修改 WAL 记录（INSERT_ROW/DELETE_ROW/UPDATE_ROW），返回 lsn；无日志时返回 INVALID_LSN
        auto LogDataChange(LogRecordType type, entry_id table_id, const RID &rid,
                           const char *image, uint32_t image_size) -> lsn_t {
            if (!log_manager_) {
                return INVALID_LSN;
            }
            tx_id_t txid = transaction_ ? transaction_->tx_id_ : INVALID_TX_ID;
            LogDataPayload p;
            p.table_id = table_id;
            p.rid = rid;
            if (image != nullptr && image_size > 0) {
                p.image.assign(image, image + image_size);
            }
            std::vector<char> encoded = p.Encode();
            char *payload = new char[encoded.size()];
            memcpy(payload, encoded.data(), encoded.size());

            LogRecordHeader header{};
            header.tx_id_ = txid;
            header.type_ = type;
            header.payload_size_ = static_cast<uint32_t>(encoded.size());
            LogRecord record(header, payload);  // LogRecord 析构释放 payload
            return log_manager_->AppendLogRecord(record);
        }
    };
}
