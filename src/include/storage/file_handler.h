//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <atomic>
#include <string>

namespace YourSQL {

    class FileHandler {
    public:
        explicit FileHandler() = default;
        ~FileHandler() = default;

        std::string file_path_;
        int fd_{};
        std::atomic<int> rec_{};
        long long last_acc_time_{};
    };

}
