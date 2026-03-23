//
// Created by huan.yang on 2026-01-28.
//
#include "parser/transformer.h"

#include <algorithm>

#include "parser/statement/create_table_statement.h"
#include "sql/DeleteStatement.h"
#include "sql/InsertStatement.h"
#include "sql/SelectStatement.h"

using namespace YourSQL;

auto Transformer::transformStatement(hsql::SQLParserResult &sqlParserResult, std::vector<std::unique_ptr<BaseStatement> > &targetStatements) -> bool {
    for (size_t index = 0L; index < sqlParserResult.size(); ++index) {
        auto stmt = sqlParserResult.getStatement(index);
        auto res = transformStatement(stmt);
        if (!res) {
            targetStatements.clear();
            return false;
        }
        targetStatements.push_back(std::move(res));
    }
    return true;
}

auto Transformer::transformStatement(const hsql::SQLStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {return nullptr;}
    switch (sql_statement->type()) {
        case hsql::kStmtSelect:
            return transformSelect((hsql::SelectStatement*)sql_statement);
        case hsql::kStmtInsert:
            return transformInsert((hsql::InsertStatement*)sql_statement);
        case hsql::kStmtCreate:
            return transformCreate((hsql::CreateStatement*)sql_statement);
        default:
            throw std::runtime_error("Invalid SQL statement");
    }
}


auto Transformer::transformCreate(const hsql::CreateStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {
        throw std::runtime_error("Transformer::transformCreate args sql_statement is nullptr;");
    }
    switch (sql_statement->type) {
        case hsql::kCreateTable:
            return transformCreateTable(sql_statement);
        default: throw std::runtime_error("Transformer::tansformCreate unknow sql statement type.");
    }
}

auto Transformer::transformCreateTable(const hsql::CreateStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    auto table_statement = std::make_unique<CreateTableStatement>();
    table_statement->table_name_ = sql_statement->tableName;
    auto column_size = sql_statement->columns->size();
    for (size_t i = 0; i < column_size; ++i) {
        auto c  =sql_statement->columns->at(i);
        Column column;
        column.name_ = c->name;
        column.column_types_ = CreateTableStatement::TransformerType(c->type);
        column.not_null_ = c->nullable;
        table_statement->columns_.push_back(std::move(column));
    }
    return table_statement;
}
