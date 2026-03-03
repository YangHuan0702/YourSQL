//
// Created by huan.yang on 2026-03-03.
//
#include "storage/posix_disk_manager.h"


#include "common/macro.h"

using namespace YourSQL;

PosixDiskManager::PosixDiskManager() : fs_(std::string(DATA_PATH) + "/" + std::string(DATA_FILE_NAME),
                                           std::ios::in | std::ios::binary | std::ios::out) {
}

auto PosixDiskManager::Open() -> void {
}

auto PosixDiskManager::Close() -> void {
    fs_.close();
}


auto PosixDiskManager::Read(page_id_t page_id, Page *page) -> void {
    if (!fs_.is_open()) {
        throw std::runtime_error("PosixDiskManager::Read: File is not open");
    }
    fs_.seekg(page_id * PAGE_SIZE, std::ios::beg);
    fs_.read(page->data_,PAGE_SIZE);
}


auto PosixDiskManager::Write(Page *page) -> void {
    if (!fs_.is_open()) {
        throw std::runtime_error("PosixDiskManager::Write: File is not open");
    }
    page_id_t page_id = page->id_;
    fs_.seekp(page_id * PAGE_SIZE, std::ios::beg);
    fs_.write(page->data_,PAGE_SIZE);
}
