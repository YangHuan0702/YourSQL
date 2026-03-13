//
// Created by huan.yang on 2026-03-13.
//
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "binder/binder.h"
#include "executor/execute.h"
#include "executor/executor_factory.h"
#include "glog/logging.h"
#include "parser/statement/insert_statement.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"
#include "storage/page/row.h"

using namespace YourSQL;

#define SLOT_SIZE (sizeof(uint16_t) * 2 + 1)

TEST(Executor,InsertAfterReadTupleTest) {
    auto disk = std::make_unique<PosixDiskManager>();

    page_id_t page_id = 1;
    Page page;
    disk->Read(page_id,&page);


    size_t slot_offset = PAGE_SIZE - (SLOT_SIZE * page_id);
    uint16_t offset;
    uint16_t size;
    char deleted;
    memcpy(&offset,page.data_+slot_offset,sizeof(uint16_t));
    memcpy(&size,page.data_+slot_offset + sizeof(uint16_t),sizeof(uint16_t));
    memcpy(&deleted,page.data_+slot_offset + sizeof(uint16_t)*2,sizeof(char));
    std::cout << "offset: " << offset << " size: " << size << " deleted: " << deleted << std::endl;

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);

    std::string name = "name";
    auto column_name = ColumnEntry(table->GetNextColumnId(), name, ColumnTypes::VARCHAR);
    table->AddColumn(column_name);

    std::string age = "age";
    auto column_age = ColumnEntry(table->GetNextColumnId(), age, ColumnTypes::INTEGER);
    table->AddColumn(column_age);

    std::string email = "email";
    auto column_email = ColumnEntry(table->GetNextColumnId(), email, ColumnTypes::VARCHAR);
    table->AddColumn(column_email);

    std::string del = "del";
    auto column_del = ColumnEntry(table->GetNextColumnId(), del, ColumnTypes::INTEGER);
    column_del.default_value_ = Value(0);
    table->AddColumn(column_del);

    catalog->AddTable(std::move(table));

    auto &table_entry = catalog->tables_[catalog->table_name_idx_["user"]];
    Schema sc = table_entry->GetSchema();

    Tuple tuple;
    tuple.data_ = new char[size];
    tuple.schema_ = sc;
    tuple.tuple_size_ = size;
    memcpy(tuple.data_, page.data_ + offset, size);

    Row row(sc);
    row.Deserialize(tuple);

    Value value_name = row.GetValue(0);
    Value value_age = row.GetValue(1);
    std::cout << "name: " << value_name.GetString() << " , age: "<< value_age.GetInt() << std::endl;

    delete [] tuple.data_;
}
