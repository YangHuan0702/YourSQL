//
// Created by huan.yang on 2026-02-12.
//
#pragma once
#include <utility>
#include <variant>
#include <string>

namespace YourSQL {

    enum class ColumnTypes {
        INVALID,
        INTEGER,
        BOOL,
        VARCHAR,
        VARCHAR2,
        TIMESTAMP
    };


    class Value {
    public:
        explicit Value(std::variant<int,bool,std::string,long long> val) : val_(std::move(val))  {}

        auto GetInt() const -> int { return std::get<int>(val_);}
        auto GetBool() const -> bool { return std::get<bool>(val_);}
        auto GetString() const -> std::string { return std::get<std::string>(val_);}
        auto GetTimestamp() const -> long long { return std::get<long long>(val_);}

    private:
        std::variant<int,bool,std::string,long long> val_;
    };


}
