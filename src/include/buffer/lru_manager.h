//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <list>
#include <mutex>
#include <unordered_map>
namespace YourSQL {

    class LRUManager {
    public:
        explicit LRUManager() {

        }
        ~LRUManager() = default;

        auto Pin(int frame) -> void {
            std::unique_lock<std::mutex> lock(mutex_);
            pin_count_[frame]++;

            if (map_.find(frame) != map_.end()) {
                list_.erase(map_[frame]);
                map_.erase(frame);
            }
        }

        auto UnPin(int frame) -> void {
            std::unique_lock<std::mutex> lock(mutex_);
            pin_count_[frame]--;
            if (pin_count_[frame] == 0) {
                list_.push_front(frame);
                map_[frame] = list_.begin();
            }
        }

        // 标记页面被访问，如果页面在 LRU 列表中，将其移到头部
        auto Touch(int frame) -> void {
            std::unique_lock<std::mutex> lock(mutex_);
            if (map_.find(frame) != map_.end()) {
                list_.erase(map_[frame]);
                list_.push_front(frame);
                map_[frame] = list_.begin();
            }
        }

        auto GetPinCount(int frame) -> int {
            return pin_count_[frame];
        }

        auto Evict() -> int {
            std::unique_lock<std::mutex> lock(mutex_);
            if (list_.empty()) {
                throw std::runtime_error("LRU manager is empty");
            }
            return list_.back();
        }

    private:
        std::unordered_map<int,int> pin_count_;
        std::list<int> list_;
        std::unordered_map<int, std::list<int>::iterator> map_;
        std::mutex mutex_;
    };

}
