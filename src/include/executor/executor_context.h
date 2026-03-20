//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "catalog/catalog.h"
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
                                 const std::shared_ptr<UndoLogManager> &undo_log_manager) : catalog_(catalog),
            buffer_manager_(buffer_manager),
            meta_page_(meta_page), transaction_manager_(transaction_manager), transaction_(transaction),
            undo_log_manager_(undo_log_manager) {
        }

        ~ExecutorContext() = default;

        std::shared_ptr<Catalog> catalog_;

        std::shared_ptr<BufferManager> buffer_manager_;

        std::shared_ptr<MetaPage> meta_page_;


        std::shared_ptr<TransactionManager> transaction_manager_;
        std::shared_ptr<Transaction> transaction_;
        std::shared_ptr<UndoLogManager> undo_log_manager_;
    };
}
