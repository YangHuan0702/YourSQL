//
// Created by huan.yang on 2026-03-03.
//
#include "buffer/buffer_manager.h"

#include "common/util/page_id_util.h"

using namespace YourSQL;


auto BufferManager::ReadPage(page_id_t page_id, Page *page) -> bool {
    if (!page) return false;
    disk_manager_->Read(page_id, page);
    page->SetDirty(false);
    return true;
}

auto BufferManager::Flush(page_id_t page_id) -> void {
    std::lock_guard lock(mutex_);
    if (buffer_pages_.find(page_id) == buffer_pages_.end()) {
        throw std::runtime_error("BufferManager::Flush: Page not found");
    }
    auto page = &frames_[buffer_pages_[page_id]];
    if (page->is_dirty_) {
        disk_manager_->Write(page);
        page->is_dirty_ = false;
    }
}

auto BufferManager::Release(page_id_t page_id) -> void {
    std::lock_guard lock(mutex_);
    if (buffer_pages_.find(page_id) == buffer_pages_.end()) {
        throw std::runtime_error("BufferManager::Release: Page not found");
    }
    int frame_id = buffer_pages_[page_id];
    lru_manager_->UnPin(frame_id);

    if (frames_[frame_id].is_dirty_) {
        disk_manager_->Write(&frames_[frame_id]);
        frames_[frame_id].is_dirty_ = false;
    }

    if (lru_manager_->GetPinCount(frame_id) == 0) {
        free_pages_.insert(free_pages_.begin(),frame_id);
        buffer_pages_.erase(page_id);
    }
}

auto BufferManager::NewPage() -> Page * {
    std::lock_guard guard(mutex_);
    Page *ans = nullptr;
    int frame = 0;
    if (!free_pages_.empty()) {
        frame = free_pages_.back();
        free_pages_.pop_back();

        lru_manager_->Pin(frame);
        ans = &frames_[frame];
        if (frames_[frame].is_dirty_) {
            disk_manager_->Write(&frames_[frame]);
        }
    } else {
        frame = lru_manager_->Evict();
        lru_manager_->Pin(frame);
        ans = &frames_[frame];
        if (ans->is_dirty_) {
            // write to disk
            disk_manager_->Write(ans);
        }
    }
    ans->Reset();
    ans->id_ = PageIdUtil::GetNextPageId();
    ans->is_dirty_ = false;
    buffer_pages_[ans->id_] = frame;
    return ans;
}


auto BufferManager::Release(Page *page) -> void {
    if (!page) return;
    Release(page->id_);
}


auto BufferManager::FetchPage(page_id_t page_id) -> Page * {
    std::lock_guard guard(mutex_);

    Page *ans = nullptr;
    if (buffer_pages_.find(page_id) != buffer_pages_.end()) {
        lru_manager_->Pin(buffer_pages_[page_id]);
        ans = &frames_[buffer_pages_[page_id]];
    } else {
        if (!free_pages_.empty()) {
            int frame = free_pages_.back();
            free_pages_.pop_back();

            if (frames_[frame].is_dirty_) {
                disk_manager_->Write(&frames_[frame]);
            }
            disk_manager_->Read(page_id, &frames_[frame]);
            lru_manager_->Pin(frame);
            ans = &frames_[frame];
            buffer_pages_[page_id] = frame;
        } else {
            int frame = lru_manager_->Evict();
            ans = &frames_[frame];
            if (ans->is_dirty_) {
                // write to disk
                disk_manager_->Write(ans);
            }
            bool r = ReadPage(page_id, ans);
            if (!r) {
                throw std::runtime_error("Failed to read page");
            }
            lru_manager_->Pin(frame);
            buffer_pages_[page_id] = frame;
        }
    }
    ans->SetPageId(page_id);
    return ans;
}

auto BufferManager::TouchPage(page_id_t page_id) -> void {
    std::lock_guard lock(mutex_);
    if (buffer_pages_.find(page_id) == buffer_pages_.end()) {
       throw std::runtime_error("BufferManager::TouchPage: Page not found");
    }
    int frame_id = buffer_pages_[page_id];
    lru_manager_->Touch(frame_id);
}
