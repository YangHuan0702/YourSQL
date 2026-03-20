//
// Created by huan.yang on 2026-03-20.
//

#include "catalog/catalog.h"
#include "gtest/gtest.h"
#include "parser/parser.h"
#include "storage/posix_disk_manager.h"

using namespace YourSQL;

TEST(Transaction,TransactionInsert) {
    std::string sql = "insert into user (name,age) values('你好',23)";

    Parser parser;
    parser.ParserSQL(sql);

    auto catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);
    auto table_id = table->id_;

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

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_shared<MetaPage>(buffer_manager);



}