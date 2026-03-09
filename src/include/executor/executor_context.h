//
// Created by huan.yang on 2026-03-06.
//
#pragma once
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "catalog/catalog.h"

namespace YourSQL {
    class ExecutorContext {
    public:
        explicit ExecutorContext(const std::shared_ptr<Catalog> &catalog,
                                 const std::shared_ptr<BufferManager> &buffer_manager,
                                 const std::shared_ptr<MetaPage> &meta_page) : catalog_(catalog),
                                                                               buffer_manager_(buffer_manager),
                                                                               meta_page_(meta_page) {
        }

        ~ExecutorContext() = default;

        std::shared_ptr<Catalog> catalog_;

        std::shared_ptr<BufferManager> buffer_manager_;

        std::shared_ptr<MetaPage> meta_page_;
    };
}
