//
// Created by 杨欢 on 2026/2/14.
//

#pragma once

#include <unordered_map>

#include "table_entry.h"

namespace YourSQL {

    class Catalog {
    public:
        explicit Catalog() = default;
        ~Catalog() = default;

        std::unordered_map<std::string,std::vector<TableEntry>> databases_;
    };

}
