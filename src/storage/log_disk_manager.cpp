//
// Created by huan.yang on 2026-03-19.
//
#include "storage/log_disk_manager.h"

#include <filesystem>

using namespace YourSQL;

LogDiskManager::LogDiskManager(const std::string& file_path) : file_path_(file_path) {
    if (!std::filesystem::exists(file_path)) {
        std::ofstream create(file_path,std::ios::binary);
        create.close();
    }

    log_file.open(file_path, std::ios::binary | std::ios::out | std::ios::in | std::ios::app);

    if (!log_file.is_open()) {
        throw std::runtime_error("LogDiskManager:Could not open file " + file_path);
    }
}


auto LogDiskManager::Write(char *data, int size) -> void {
    if (!log_file.is_open()) {
        throw std::runtime_error("LogDiskManager::Write log_file is not open.");
    }
    log_file.clear();
    // 以 app 模式打开，写入总是追加到文件尾部
    log_file.seekp(0, std::ios::end);
    log_file.write(data, size);
    log_file.flush();
}


auto LogDiskManager::ReadAll() -> std::vector<char> {
    size_t sz = Size();
    std::vector<char> buf(sz);
    if (sz == 0) {
        return buf;
    }
    // 用独立输入流读取，避免与追加写共享流位置带来的问题
    std::ifstream in(file_path_, std::ios::binary);
    in.read(buf.data(), static_cast<std::streamsize>(sz));
    return buf;
}


auto LogDiskManager::Size() -> size_t {
    return std::filesystem::exists(file_path_) ? std::filesystem::file_size(file_path_) : 0;
}
