//
// Created by 杨欢 on 2026/3/22.
//
#pragma once
#include <string>

#include "sql/Expr.h"
#include "parser/statement.h"

namespace YourSQL {
    struct Column {
        std::string name_;
        ColumnTypes column_types_;
        bool not_null_;
    };


    class CreateTableStatement : public BaseStatement {
    public:
        explicit CreateTableStatement() : BaseStatement(StatementType::OBJ, StatementClassify::CREATE_TABLE) {
        }

        ~CreateTableStatement() override = default;

        auto to_string() -> std::string override {
            return "CreateTableStatement(" + table_name_ + ")";
        }

        static auto TransformerType(hsql::ColumnType type) -> ColumnTypes {
            switch (type.data_type) {
                case hsql::DataType::INT: return ColumnTypes::INTEGER;
                case hsql::DataType::VARCHAR: return ColumnTypes::VARCHAR;
                default: throw std::runtime_error("Unsupported column type");
            }
        }

        std::string table_name_;

        std::vector<Column> columns_;
    };
}
