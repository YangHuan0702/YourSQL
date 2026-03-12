//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

#include "lru_manager.h"
#include "storage/disk_manager.h"

namespace YourSQL {

    class BufferManager {
    public:
        explicit BufferManager(std::shared_ptr<DiskManger> disk_manger) : disk_manager_(std::move(disk_manger)),max_pages_(BUFFER_MAX_PAGE) {
            frames_ = new Page[max_pages_];
            lru_manager_ = std::make_unique<LRUManager>();
            for (int i = 0; i < max_pages_; i++) {
                free_pages_.push_back(i);
            }
        }
        ~BufferManager() {
            delete [] frames_;
        }

        auto NewPage() -> Page*;
        auto Release(Page *page) -> void;
        auto Release(page_id_t page_id) -> void;
        auto FetchPage(page_id_t page_id) -> Page*;
        auto Flush(page_id_t page_id) -> void;

        // 标记页面被访问，将页面移到 LRU 列表头部（如果页面未被 Pin）
        auto TouchPage(page_id_t page_id) -> void;

    private:
        auto ReadPage(page_id_t,Page *page) -> bool;

        std::shared_ptr<DiskManger> disk_manager_;
        std::unique_ptr<LRUManager> lru_manager_;
        int max_pages_;
        Page *frames_;
        std::vector<int> free_pages_;
        std::unordered_map<page_id_t, int> buffer_pages_;

        std::mutex mutex_;
    };

}
