//
// Created by 杨欢 on 2026/3/22.
//

#include <gtest/gtest.h>

#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "catalog/catalog.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;

TEST(Transaction,CatalogReadTransaction) {
    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_unique<MetaPage>(buffer_manager);

    auto catalog = std::make_shared<Catalog>(std::move(meta_page));

    for (auto &[id,table] : catalog->tables_) {
        std::cout<< "table :" << table->name_ << " columns:[ " << std::endl;
        for (auto &[cid,column] : table->columns_) {
            std::cout << " - column: " << column.name_ <<" ,id:" << column.id_ << std::endl;
        }
        std::cout << " ]"<<std::endl;
    }
}
