//
// Created by huan.yang on 2026-01-28.
//
#pragma once
#include <string>


namespace YourSQL {
    class TableRef {
    public:
        explicit TableRef(std::string table_name, std::string alias) : table_name(table_name), alias(alias) {
        }
        ~TableRef() = default;

        std::string table_name;
        std::string alias;
    };
}
