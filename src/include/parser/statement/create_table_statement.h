//
// Created by 杨欢 on 2026/3/22.
//
#pragma once
#include <string>

#include "parser/statement.h"

namespace YourSQL {

    class CreateTableStatement : public BaseStatement {
    public:
        explicit CreateTableStatement() : BaseStatement(StatementType::OBJ,StatementClassify::CREATE_TABLE) {}
        ~CreateTableStatement() override = default;


        std::string table_name_;


    };

}



