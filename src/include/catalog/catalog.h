//
// Created by 杨欢 on 2026/2/14.
//

#pragma once

#include <unordered_map>
#include "common/type.h"
#include "table_entry.h"

namespace YourSQL {

    class Catalog {
    public:
        explicit Catalog() = default;
        ~Catalog() = default;

        std::unordered_map<std::string,entry_id> table_name_idx_;
        std::unordered_map<entry_id,std::unique_ptr<TableEntry>> tables_;
    };

}
