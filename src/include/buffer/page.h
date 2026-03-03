//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <cstring>

#include "common/macro.h"
#include "common/type.h"

namespace YourSQL {
    class Page {
    public:
        Page() {
            data_ = new char[PAGE_SIZE];
        }

        explicit Page(page_id_t id) : id_(id) {
            data_ = new char[PAGE_SIZE];
            memset(&data_, 0, sizeof(data_));
        }

        Page(const Page &) = delete;

        Page &operator=(const Page &page) = default;

        ~Page() {
            delete [] data_;
        }

        auto Reset() -> void {
            memset(data_, 0, PAGE_SIZE);
            id_ = 0;
        }

        auto SetDirty(bool isDirty) -> void { this->is_dirty_ = isDirty; }

        auto SetData(char *data) const -> void {
            memset(data_, 0, PAGE_SIZE);
            memcpy(data_, data, PAGE_SIZE);
        }

        auto SetPageId(page_id_t id) -> void {
            this->id_ = id;
        }

        bool is_dirty_{false};
        page_id_t id_{};
        char *data_;
    };
}
