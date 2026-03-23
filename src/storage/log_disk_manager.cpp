//
// Created by huan.yang on 2026-03-19.
//
#include "storage/log_disk_manager.h"

#include <filesystem>

using namespace YourSQL;

LogDiskManager::LogDiskManager(const std::string& file_path) {
    if (!std::filesystem::exists(file_path)) {
        std::ofstream create(file_path,std::ios::binary);
        create.close();
    }

    log_file.open(file_path, std::ios::binary | std::ios::out | std::ios::in);

    if (!log_file.is_open()) {
        throw std::runtime_error("LogDiskManager:Could not open file " + file_path);
    }
}


auto LogDiskManager::Write(char *data, int size) -> void {
    if (!log_file.is_open()) {
        throw std::runtime_error("LogDiskManager::Write log_file is not open.");
    }
    log_file.clear();

    log_file.write(data, size);
    log_file.flush();
}

