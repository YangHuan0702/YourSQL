//
// Created by huan.yang on 2026-03-10.
//
#include "executor/executor_insert.h"
#include "catalog/table_entry.h"

using namespace YourSQL;


auto ExecutorInsert::Close() -> void {
    context_->buffer_manager_->Release(page_->GetPage()->id_);
    delete page_;

    children_[0]->Close();
}

auto ExecutorInsert::Open() -> void {
    if (context_->meta_page_->items_.find(table_id_) == context_->meta_page_->items_.end()) {
        throw std::runtime_error("ExecutorInsert::Open No meta page exists:" + std::to_string(table_id_));
    }
    MetaItem item = context_->meta_page_->items_[table_id_];
    Page *page = nullptr;
    bool read = true;
    if (item.last_page_id == INVALID_PAGE_ID) {
        page = context_->buffer_manager_->NewPage();
        read = false;
    } else {
        page = context_->buffer_manager_->FetchPage(item.last_page_id);
    }

    page_ = new TablePage(page,read);
    children_[0]->Open();
}

auto ExecutorInsert::Next(Tuple *tuple) -> bool {
    while (children_[0]->Next(tuple)) {
        RID rid;
        // 计算Page空间，如果空间不够还需要创建Page
        page_->InsertTuple(*tuple,&rid);
        context_->buffer_manager_->Flush(page_->GetPage()->id_);
    }
    return true;
}
