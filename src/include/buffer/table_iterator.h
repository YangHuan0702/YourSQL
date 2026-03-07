//
// Created by huan.yang on 2026-03-07.
//
#pragma once
#include "catalog/table_entry.h"
#include "storage/page/table_page.h"

namespace YourSQL {

    class TableIterator {
    public:
        explicit TableIterator() = default;
        ~TableIterator() = default;

        auto operator*() ->Tuple {

        }

        auto operator++() -> TableIterator& {
            index_++;
            return *this;
        }

        auto operator!=(const TableIterator &table_iterator) -> bool {
            return table_iterator.index_ != index_;
        }


        size_t index_{0};

    };

}
