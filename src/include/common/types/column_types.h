//
// Created by huan.yang on 2026-02-12.
//
#pragma once
#include <iostream>
#include <ostream>
#include <utility>
#include <variant>
#include <string>

namespace YourSQL {
    enum class ColumnTypes {
        INVALID,
        INTEGER,
        BOOL,
        DOUBLE,
        VARCHAR,
        VARCHAR2,
        TIMESTAMP,
        NULLTYPE
    };


    class Value {
    public:
        Value() : is_null(true) {
        }

        explicit Value(std::variant<int, bool, std::string, long long, double> val) : val_(std::move(val)) {
        }

        [[nodiscard]] auto GetInt() const -> int { return std::get<int>(val_); }
        [[nodiscard]] auto GetBool() const -> bool { return std::get<bool>(val_); }
        [[nodiscard]] auto GetString() const -> std::string { return std::get<std::string>(val_); }
        [[nodiscard]] auto GetTimestamp() const -> long long { return std::get<long long>(val_); }
        [[nodiscard]] auto GetDouble() const -> double { return std::get<double>(val_); }
        [[nodiscard]] auto IsNull() const -> bool { return is_null; }
        auto IsInt() const -> bool { return std::holds_alternative<int>(val_); }
        auto IsLongLong() const -> bool { return std::holds_alternative<long long>(val_); }
        auto IsDouble() const -> bool { return std::holds_alternative<double>(val_); }
        auto IsString() const -> bool { return std::holds_alternative<std::string>(val_); }

        auto PrintVal() const -> void {
            std::visit([](auto &&arg) {
                std::cout << arg << std::endl; // 假设 arg 支持 <<
            }, val_);
        }

    private:
        bool is_null = false;
        std::variant<int, bool, std::string, long long, double> val_;
    };
}
