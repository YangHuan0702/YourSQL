//
// Created by huan.yang on 2026-03-23.
//
#include "binder/binder.h"

using namespace YourSQL;



auto Binder::BoundCreateTableStatement(std::unique_ptr<CreateTableStatement> parser_statement) -> std::unique_ptr<YourSQL::BoundCreateTableStatement> {
    if (catalog_->table_name_idx_.find(parser_statement->table_name_) != catalog_->table_name_idx_.end()) {
        throw std::runtime_error("Binder::BoundCreateTable target table is exist.");
    }

    auto table_entry = std::make_unique<TableEntry>(IdManager::GetNextEntryId(),parser_statement->table_name_);

    for (const auto& column : parser_statement->columns_) {
        ColumnEntry column_entry(table_entry->GetNextColumnId(),column.name_,column.column_types_);
        table_entry->AddColumn(column_entry);
    }
    auto bound_create_table_statement = std::make_unique<class BoundCreateTableStatement>(table_entry->id_);
    catalog_->AddTable(std::move(table_entry));

    return bound_create_table_statement;
}
