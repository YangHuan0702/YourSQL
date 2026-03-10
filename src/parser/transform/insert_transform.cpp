//
// Created by huan.yang on 2026-03-09.
//
#include "parser/transformer.h"

using namespace YourSQL;

auto Transformer::transformInsert(hsql::InsertStatement *statement) -> std::unique_ptr<InsertStatement> {
    auto insert_statement = std::make_unique<InsertStatement>();

    insert_statement->table_name_ = statement->tableName;
    size_t size = statement->columns->size();
    for (size_t index = 0; index < size; ++index) {
        std::string column_name = statement->columns->at(index);
        insert_statement->column_name_.push_back(column_name);

        switch (statement->values->at(index)->type) {
            case hsql::kExprLiteralString: {
                std::string val = statement->values->at(index)->name;
                insert_statement->values_.emplace_back(val);
                break;
            }
            case hsql::kExprLiteralInt: {
                int val = statement->values->at(index)->ival;
                insert_statement->values_.emplace_back(val);
                break;
            }
            case hsql::kExprLiteralFloat: {
                double val = statement->values->at(index)->fval;
                insert_statement->values_.emplace_back(val);
            }
            default: throw std::runtime_error("Transformer::transformInsert: unknown column type");
        }
        // insert_statement->values_.push_back();
    }
    return insert_statement;
}
