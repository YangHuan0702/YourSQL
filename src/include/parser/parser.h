//
// Created by huan.yang on 2026-01-28.
//
#pragma once

#include <memory>
#include <vector>
#include <string>

#include "statement.h"

namespace YourSQL {

    class Parser {
    public:
        Parser() = default;
        ~Parser() = default;

        auto ParserSQL(const std::string &sql) -> void;
        auto GetStatements() -> std::vector<std::unique_ptr<BaseStatement>>& {
            return statements;
        }
    private:
        std::vector<std::unique_ptr<BaseStatement>> statements;
    };

}