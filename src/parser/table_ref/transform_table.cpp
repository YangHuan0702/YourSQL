//
// Created by huan.yang on 2026-01-30.
//
#include "parser/transformer.h"

using namespace YourSQL;

auto Transformer::transformTableRef(hsql::TableRef *table_ref) -> std::unique_ptr<YourTable> {
    if (!table_ref) { return nullptr;}

    switch (table_ref->type) {
        case hsql::kTableName: {
            std::string table_name = table_ref->name ? std::string(table_ref->name) : "";
            return std::make_unique<YourTable>(table_name);
        }
        default:
            throw std::invalid_argument("Invalid table type");
    }
}

