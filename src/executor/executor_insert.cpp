//
// Created by huan.yang on 2026-03-10.
//
#include "executor/executor_insert.h"

#include "catalog/table_entry.h"
#include "common/constant.h"
#include "common/macro.h"

using namespace YourSQL;


auto ExecutorInsert::Close() -> void {
    if (page_ != nullptr) {
        context_->buffer_manager_->Release(page_->GetPage()->id_);
        delete page_;
        page_ = nullptr;
    }
    children_[0]->Close();
}

auto ExecutorInsert::Open() -> void {
    if (context_->meta_page_->items_.find(table_id_) == context_->meta_page_->items_.end()) {
        throw std::runtime_error("ExecutorInsert::Open No meta page exists:" + std::to_string(table_id_));
    }
    MetaItem &item = context_->meta_page_->items_[table_id_];

    Page *page = nullptr;
    bool read = true;
    if (item.last_page_id == INVALID_PAGE_ID) {
        // 尚无数据页：惰性分配首页
        page = context_->buffer_manager_->NewPage();
        read = false;

        context_->meta_page_->UpdateTableLastId(item.table_id_, page->id_);
        if (item.first_page_id == INVALID_PAGE_ID) {
            context_->meta_page_->UpdateTableFirstId(item.table_id_, page->id_);
        }
    } else {
        page = context_->buffer_manager_->FetchPage(item.last_page_id);
    }

    page_ = new TablePage(context_->meta_page_, table_id_, page, read);
    children_[0]->Open();
}

auto ExecutorInsert::Next(Tuple *tuple) -> bool {
    while (children_[0]->Next(tuple)) {
        RID rid{};

        // 当前页空间不足，则分配新页并链接到页链表尾部
        if (!page_->HasSpaceFor(tuple->tuple_size_)) {
            AllocateAndLinkNewPage();
        }

        if (!page_->InsertTuple(*tuple, &rid)) {
            // 即使是空的新页也放不下，说明单条记录超过页容量
            throw std::runtime_error("ExecutorInsert::Next tuple too large for an empty page");
        }

        // 写 WAL（after-image = 插入的记录字节），并把 lsn 记到页头
        lsn_t lsn = context_->LogDataChange(LogRecordType::INSERT_ROW, table_id_, rid,
                                            tuple->data_, tuple->tuple_size_);
        if (lsn != INVALID_LSN) {
            page_->SetLsn(lsn);
        }
        context_->buffer_manager_->Flush(page_->GetPage()->id_);

        // 记录写集：插入的新行，回滚时置 dead
        if (context_->transaction_) {
            WriteRecord wr;
            wr.rid = rid;
            wr.type = WriteType::INSERT;
            wr.table_id = table_id_;
            context_->transaction_->write_set_.push_back(wr);
        }
    }
    return true;
}

auto ExecutorInsert::AllocateAndLinkNewPage() -> void {
    Page *old_page = page_->GetPage();
    page_id_t old_page_id = old_page->id_;

    // 分配新页
    Page *new_page = context_->buffer_manager_->NewPage();
    page_id_t new_page_id = new_page->id_;

    // 旧页头写入 next_page_id 并落盘，维护页链表
    page_->SetNextPageId(new_page_id);
    context_->buffer_manager_->Flush(old_page_id);
    context_->buffer_manager_->Release(old_page_id);
    delete page_;

    // 切换到新页（初始化页头）
    page_ = new TablePage(context_->meta_page_, table_id_, new_page, false);

    // 更新表的 last_page_id
    context_->meta_page_->UpdateTableLastId(table_id_, new_page_id);
}
