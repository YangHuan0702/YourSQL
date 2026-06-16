//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <fstream>
#include <iosfwd>
#include <string>
#include <vector>

namespace YourSQL {


    class LogDiskManager {
    public:
        explicit LogDiskManager(const std::string& file_path);
        ~LogDiskManager() {
            if (log_file.is_open()) {
                log_file.close();
            }
        }

        // 追加写到日志文件尾部
        auto Write(char *, int) -> void;

        // 顺序读取整个日志文件（恢复用）
        auto ReadAll() -> std::vector<char>;

        // 当前日志文件大小
        auto Size() -> size_t;

    private:
        std::string file_path_;
        std::fstream log_file;
    };

}
