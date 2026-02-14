//
// Created by huan.yang on 2026-01-30.
//
#pragma once
#include <string>

namespace YourSQL {

    class YourTable {
    public:
        YourTable(std::string &name): table_name_(name) {}

        std::string table_name_;
    };

}
