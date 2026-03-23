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

    std::shared_ptr<Catalog> catalog = std::make_shared<Catalog>();
    std::string table_name = "user";
    auto table = std::make_unique<TableEntry>(IdManager::GetNextEntryId(), table_name);
    auto table_id = table->id_;

    std::string name = "name";
    auto column_name = ColumnEntry(table->GetNextColumnId(), name, ColumnTypes::VARCHAR);
    table->AddColumn(column_name);
    MetaColumnItem name_item{};
    name_item.column_name_ = name;
    name_item.type_ = ColumnTypes::VARCHAR;
    name_item.column_id_ = column_name.id_;


    std::string age = "age";
    auto column_age = ColumnEntry(table->GetNextColumnId(), age, ColumnTypes::INTEGER);
    table->AddColumn(column_age);
    MetaColumnItem age_item_;
    age_item_.column_name_ = age;
    age_item_.type_ = ColumnTypes::INTEGER;
    age_item_.column_id_ = column_age.id_;


    std::string email = "email";
    auto column_email = ColumnEntry(table->GetNextColumnId(), email, ColumnTypes::VARCHAR);
    table->AddColumn(column_email);
    MetaColumnItem email_item;
    email_item.column_name_ = email;
    email_item.type_ = ColumnTypes::VARCHAR;
    email_item.column_id_ = column_email.id_;

    std::string del = "del";
    auto column_del = ColumnEntry(table->GetNextColumnId(), del, ColumnTypes::INTEGER);
    column_del.default_value_ = Value(0);
    table->AddColumn(column_del);
    MetaColumnItem del_item;
    del_item.column_name_ = del;
    del_item.type_ = ColumnTypes::INTEGER;
    del_item.column_id_ = column_del.id_;

    catalog->AddTable(std::move(table));

    auto disk_manger = std::make_shared<PosixDiskManager>();
    auto buffer_manager = std::make_shared<BufferManager>(disk_manger);

    auto meta_page = std::make_shared<MetaPage>(buffer_manager);
    try {
        if (disk_manger->Size() == 0) {
            meta_page->Init();

            MetaItem meta_item;
            meta_item.table_id_ = table_id;
            meta_item.table_name_ = table_name;
            meta_item.last_page_id = 0;
            meta_item.first_page_id = 0;
            meta_item.num_rows_ = 0;
            meta_item.offset = sizeof(size_t) * 3;

            meta_item.items_.push_back(name_item);
            meta_item.items_.push_back(age_item_);
            meta_item.items_.push_back(email_item);
            meta_item.items_.push_back(del_item);

            meta_page->AddTable(meta_item);
        } else {
            meta_page->ReadMata();
        }
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }

}