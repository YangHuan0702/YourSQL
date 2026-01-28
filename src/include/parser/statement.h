//
// Created by huan.yang on 2026-01-28.
//

#pragma once
#include "common/types/statment_type.h"

namespace YourSQL {

    class BaseStatement {
    public :
        BaseStatement(const StatementType type,const StatementClassify classify) : classify_(classify),type_(type) {}
        virtual ~BaseStatement() =default;


        virtual auto to_string() -> std::string = 0;

        StatementClassify classify_;
        StatementType type_;
    };

}
