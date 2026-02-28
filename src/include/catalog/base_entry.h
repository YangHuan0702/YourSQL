//
// Created by 杨欢 on 2026/2/14.
//
#pragma once
#include <string>
#include "common/type.h"

namespace YourSQL {

    class BaseEntry{
    public:
        explicit BaseEntry(entry_id id,std::string &name) : id_(id),name_(std::move(name)) {}
        virtual ~BaseEntry() = default;

        virtual auto to_string() -> std::string = 0;

        entry_id id_;
        std::string name_;
    };

}
