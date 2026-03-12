//
// Created by huan.yang on 2026-03-07.
//
#include "buffer/table_iterator.h"

#include <utility>
#include "common/exception.h"
#include "storage/page/row.h"

using namespace YourSQL;

TableIterator::TableIterator(std::shared_ptr<BufferManager> buffer_manager,
                             std::shared_ptr<MetaPage> meta_page,
                             std::string table_name, entry_id table_id,
                             Schema schema)
    : buffer_manager_(std::move(buffer_manager)),
      meta_page_(std::move(meta_page)),
      table_name_(std::move(table_name)),
      schema_(std::move(schema)),
      table_id_(table_id) {
    // 获取表的第一个页面 ID
    current_page_id_ = meta_page_->GetFirstPageId(table_name_);

    if (current_page_id_ == 0) {
        // 表为空，直接设置为 end 迭代器
        is_end_ = true;
        return;
    }

    // 加载第一个页面
    LoadPage();
    current_row_index_ = 0;
}

auto TableIterator::operator*() -> Tuple {
    if (is_end_) {
        throw std::runtime_error("TableIterator::operator*() is not valid");
    }

    RID rid;
    rid.page_id_ = current_page_id_;
    rid.row_id_ = current_row_index_;

    Tuple tuple;
    tuple.schema_ = schema_;
    tuple.data_ = current_table_page_->GetPage()->data_;
    return tuple;
}

auto TableIterator::operator++() -> TableIterator & {
    if (is_end_) {
        return *this;
    }

    current_row_index_++;

    // 检查是否需要换页
    if (current_row_index_ >= current_page_num_rows_) {
        // 获取下一页的 page_id
        page_id_t next_page_id = current_table_page_->GetPage()->next_page_id_;
        buffer_manager_->Release(current_page_id_);
        if (next_page_id == 0) {
            // 没有下一页了，到达末尾
            is_end_ = true;
        } else {
            // 移动到下一页
            current_page_id_ = next_page_id;
            LoadPage();
            current_row_index_ = 0;
        }
    } else {
        buffer_manager_->TouchPage(current_page_id_);
    }

    return *this;
}

auto TableIterator::LoadPage() -> void {
    Page *page = buffer_manager_->FetchPage(current_page_id_);

    current_table_page_ = std::make_unique<TablePage>(meta_page_,table_id_, page, true);
    current_page_num_rows_ = current_table_page_->GetCount();
}
