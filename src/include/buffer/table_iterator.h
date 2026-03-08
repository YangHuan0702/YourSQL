//
// Created by huan.yang on 2026-03-07.
//
#pragma once
#include <memory>

#include "catalog/table_entry.h"
#include "storage/page/table_page.h"
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"

namespace YourSQL {

    class TableIterator {
    public:
        // 默认构造函数，用于构造 end 迭代器
        explicit TableIterator() : is_end_(true) {}

        // 正常构造函数，用于构造 begin 迭代器
        explicit TableIterator(std::shared_ptr<BufferManager> buffer_manager,
                              std::shared_ptr<MetaPage> meta_page,
                              const std::string &table_name,
                              const Schema &schema);

        ~TableIterator() = default;

        auto operator*() -> Tuple;

        auto operator++() -> TableIterator&;

        auto operator==(const TableIterator &other) const -> bool {
            return is_end_ == other.is_end_ &&
                   current_page_id_ == other.current_page_id_ &&
                   current_row_index_ == other.current_row_index_;
        }

        auto operator!=(const TableIterator &other) const -> bool {
            return !(*this == other);
        }

    private:
        auto LoadPage() -> void;

        std::shared_ptr<BufferManager> buffer_manager_;
        std::shared_ptr<MetaPage> meta_page_;
        std::string table_name_;
        Schema schema_;

        page_id_t current_page_id_{0};
        uint32_t current_row_index_{0};
        uint32_t current_page_num_rows_{0};
        std::unique_ptr<TablePage> current_table_page_;

        bool is_end_{false};
    };

}
