//
// Created by huan.yang on 2026-03-10.
//
#include "executor/executor_insert.h"
#include "catalog/catalog.h"
#include "catalog/table_entry.h"
#include "common/types/column_types.h"
#include <cstring>

using namespace YourSQL;


auto ExecutorInsert::Close() -> void {
    if (page_) {
        delete page_;
    }
    children_[0]->Close();
}

auto ExecutorInsert::Open() -> void {
    if (context_->meta_page_->items_.find(table_id_) == context_->meta_page_->items_.end()) {
        throw std::runtime_error("No meta page exists");
    }
    MetaItem item = context_->meta_page_->items_[table_id_];
    Page *page = context_->buffer_manager_->FetchPage(item.last_page_id);

    page_ = new TablePage(page);

    children_[0]->Open();
}

auto ExecutorInsert::Next(Tuple *tuple) -> bool {
    Tuple tmp ;
    while (children_[0]->Next(tuple)) {
        RID rid;
        page_->InsertTuple(tmp,&rid);
        context_->buffer_manager_->Flush(page_->GetPage()->id_);
    }
    return true;
}
