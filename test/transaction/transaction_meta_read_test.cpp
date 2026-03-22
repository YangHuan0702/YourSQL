//
// Created by 杨欢 on 2026/3/21.
//
#include "buffer/meta_page.h"
#include "gtest/gtest.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;


TEST(Transaction,TransactionMetaReadTest) {

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_shared<MetaPage>(buffer_manager);

    meta_page->ReadMata();

    std::cout << "-----------------------" << std::endl;
    for (const auto &[id,item] : meta_page->items_) {
        std::string table_name = item.table_name_;

        std::cout << table_name << ": "<< std::endl;

        if (!item.items_.empty()) {
            for (const auto& meta_column_item : item.items_) {
                std::cout << "++++++++++++" << std::endl;
                std::cout << "[ " << meta_column_item.column_name_ << " ]" << std::endl;
            }
        }
    }

}