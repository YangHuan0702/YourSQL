//
// Created by huan.yang on 2026-03-19.
//
#pragma once
#include <fstream>
#include <iosfwd>

namespace YourSQL {


    class LogDiskManager {
    public:
        explicit LogDiskManager(const std::string& file_path);
        ~LogDiskManager() {
            if (log_file.is_open()) {
                log_file.close();
            }
        }

        auto Write(char *,int) -> void;

    private:
        std::fstream log_file;
    };

}
