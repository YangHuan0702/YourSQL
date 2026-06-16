//
// Created by huan.yang on 2026-03-22.
//
#include "parser/transformer.h"
#include "parser/statement/delete_statement.h"
#include "parser/statement/update_statement.h"
#include "parser/statement/transaction_statement.h"

#include "sql/DeleteStatement.h"
#include "sql/UpdateStatement.h"
#include "sql/TransactionStatement.h"
#include "sql/Expr.h"

using namespace YourSQL;

namespace {
    // 把 hsql 字面量表达式转为 Value
    auto LiteralToValue(const hsql::Expr *expr) -> Value {
        switch (expr->type) {
            case hsql::kExprLiteralString:
                return Value(std::string(expr->name));
            case hsql::kExprLiteralInt:
                return Value(static_cast<int>(expr->ival));
            case hsql::kExprLiteralFloat:
                return Value(expr->fval);
            default:
                throw std::runtime_error("Transformer: unsupported literal in SET/value clause");
        }
    }
}

auto Transformer::transformDelete(const hsql::DeleteStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {
        throw std::runtime_error("Transformer::transformDelete: statement is nullptr");
    }
    auto statement = std::make_unique<DeleteStatement>();
    statement->table_name_ = sql_statement->tableName ? std::string(sql_statement->tableName) : "";

    auto where = transformWhere(sql_statement->expr);
    statement->SetWhereExpr(where);
    return statement;
}

auto Transformer::transformUpdate(const hsql::UpdateStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {
        throw std::runtime_error("Transformer::transformUpdate: statement is nullptr");
    }
    auto statement = std::make_unique<UpdateStatement>();
    statement->table_name_ =
        (sql_statement->table && sql_statement->table->name) ? std::string(sql_statement->table->name) : "";

    if (sql_statement->updates) {
        for (auto *clause : *sql_statement->updates) {
            UpdateSetClause set_clause;
            set_clause.column_name_ = clause->column ? std::string(clause->column) : "";
            set_clause.value_ = LiteralToValue(clause->value);
            statement->set_clauses_.push_back(std::move(set_clause));
        }
    }

    auto where = transformWhere(sql_statement->where);
    statement->SetWhereExpr(where);
    return statement;
}

auto Transformer::transformTransaction(const hsql::TransactionStatement *sql_statement) -> std::unique_ptr<BaseStatement> {
    if (!sql_statement) {
        throw std::runtime_error("Transformer::transformTransaction: statement is nullptr");
    }
    TxnCommand cmd;
    switch (sql_statement->command) {
        case hsql::kBeginTransaction: cmd = TxnCommand::BEGIN; break;
        case hsql::kCommitTransaction: cmd = TxnCommand::COMMIT; break;
        case hsql::kRollbackTransaction: cmd = TxnCommand::ROLLBACK; break;
        default: throw std::runtime_error("Transformer::transformTransaction: unknown command");
    }
    return std::make_unique<TransactionStatement>(cmd);
}
