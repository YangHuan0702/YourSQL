//
// Created by huan.yang on 2026-03-03.
//
#pragma once
#include <fstream>
#include <mutex>

#include "disk_manager.h"

namespace YourSQL {
    class PosixDiskManager final : public DiskManger {
    public:
        explicit PosixDiskManager();

        ~PosixDiskManager() override {
            Close();
        }

        auto Size() -> size_t override;

        auto Open() -> void override;

        auto Close() -> void override;

        auto Read(page_id_t page_id, Page *page) -> void override;

        auto Write(Page *page) -> void override;

    private:
        std::string file_path_;
        std::fstream fs_;
        std::mutex mutex_;
    };
}
