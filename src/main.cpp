//
// Created by YourSQL Terminal
//

#include <iostream>
#include <string>
#include <memory>

#include "binder/binder.h"
#include "catalog/catalog.h"
#include "executor/execute.h"
#include "executor/executor_context.h"
#include "executor/executor_factory.h"
#include "parser/parser.h"
#include "planner/planner.h"
#include "storage/posix_disk_manager.h"
#include "buffer/buffer_manager.h"
#include "buffer/meta_page.h"
#include "client/client_session.h"
#include "common/macro.h"
#include "log/log_manager.h"
#include "log/log_buffer.h"
#include "storage/log_disk_manager.h"
#include "parser/statement/transaction_statement.h"
#include "transaction/transaction_manager.h"
#include "transaction/recovery_manager.h"
#include "transaction/undo_log_manager.h"

using namespace YourSQL;

class YourSQLTerminal {
public:
    YourSQLTerminal() {
        // 初始化各个组件
        catalog_ = std::make_shared<Catalog>();
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
        meta_page_ = std::make_shared<MetaPage>(buffer_manager_);
        transaction_manager_ = std::make_shared<TransactionManager>();
        transaction_manager_->SetStorage(buffer_manager_.get(), meta_page_.get());

        // 崩溃恢复：用独立的日志读取器扫描 WAL，重建事务终态（Analysis+Redo）
        std::string wal_path = std::string(DATA_PATH) + "/ydb.wal";
        {
            auto recover_disk = std::make_shared<LogDiskManager>(wal_path);
            RecoveryManager recovery(recover_disk, buffer_manager_, meta_page_, transaction_manager_);
            size_t replayed = recovery.Recover();
            if (replayed > 0) {
                std::cout << "Recovery: replayed " << replayed << " log record(s)" << std::endl;
            }
        }

        // WAL：日志盘 + 日志缓冲 + 日志管理器（写路径）
        auto log_disk = std::make_unique<LogDiskManager>(wal_path);
        auto log_buffer = std::make_unique<LogBuffer>(LOG_BUFFER_SIZE, std::move(log_disk));
        log_manager_ = std::make_shared<LogManager>(std::move(log_buffer));
        transaction_manager_->SetLogManager(log_manager_.get());

        undo_log_manager_ = std::make_shared<UndoLogManager>(buffer_manager_);
    }

    void Run() {
        std::cout << "========================================" << std::endl;
        std::cout << "      Welcome to YourSQL Terminal       " << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Type 'exit' or 'quit' to exit" << std::endl;
        std::cout << std::endl;

        std::string sql;
        while (true) {
            std::cout << "YourSQL> ";
            std::getline(std::cin, sql);

            // 去除首尾空白字符
            Trim(sql);

            // 空行继续
            if (sql.empty()) {
                continue;
            }

            // 退出命令
            if (sql == "exit" || sql == "quit") {
                std::cout << "Goodbye!" << std::endl;
                break;
            }

            // 帮助命令
            if (sql == "help" || sql == "?") {
                PrintHelp();
                continue;
            }

            // 执行SQL
            try {
                ExecuteSQL(sql);
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }

            std::cout << std::endl;
        }
    }

private:
    void ExecuteSQL(const std::string &sql) {
        // 1. 解析SQL
        Parser parser;
        parser.ParserSQL(sql);

        auto &statements = parser.GetStatements();
        if (statements.empty()) {
            std::cerr << "No statement found" << std::endl;
            return;
        }

        // 2. 绑定
        Binder binder(catalog_);
        std::unique_ptr<BoundStatement> bound_statement;

        auto &statement = statements[0];

        // 事务控制语句：BEGIN / COMMIT / ROLLBACK，直接驱动会话，不进 binder/planner
        if (auto *txn_stmt = dynamic_cast<TransactionStatement*>(statement.get())) {
            HandleTransactionStatement(txn_stmt->command_);
            return;
        }

        // 2. 绑定
        Binder binder(catalog_);
        std::unique_ptr<BoundStatement> bound_statement;

        // 根据语句类型进行绑定
        if (auto *select_stmt = dynamic_cast<SelectStatement*>(statement.get())) {
            statement.release();
            auto useStatement = std::unique_ptr<SelectStatement>(select_stmt);
            auto bound_select = binder.BoundSelectStatement(std::move(useStatement));
            bound_statement = std::unique_ptr<BoundStatement>(bound_select.release());
        } else if (auto *insert_stmt = dynamic_cast<InsertStatement*>(statement.get())) {
            statement.release();
            auto useStatement = std::unique_ptr<InsertStatement>(insert_stmt);
            auto bound_insert = binder.BoundInsertStatement(std::move(useStatement));
            bound_statement = std::unique_ptr<BoundStatement>(bound_insert.release());
        } else if (auto *create_stmt = dynamic_cast<CreateTableStatement*>(statement.get())) {
            statement.release();
            auto useStatement = std::unique_ptr<CreateTableStatement>(create_stmt);
            auto bound_create = binder.BoundCreateTableStatement(std::move(useStatement));
            bound_statement = std::unique_ptr<BoundStatement>(bound_create.release());
        } else if (auto *delete_stmt = dynamic_cast<DeleteStatement*>(statement.get())) {
            statement.release();
            auto useStatement = std::unique_ptr<DeleteStatement>(delete_stmt);
            auto bound_delete = binder.BoundDeleteStatement(std::move(useStatement));
            bound_statement = std::unique_ptr<BoundStatement>(bound_delete.release());
        } else if (auto *update_stmt = dynamic_cast<UpdateStatement*>(statement.get())) {
            statement.release();
            auto useStatement = std::unique_ptr<UpdateStatement>(update_stmt);
            auto bound_update = binder.BoundUpdateStatement(std::move(useStatement));
            bound_statement = std::unique_ptr<BoundStatement>(bound_update.release());
        } else {
            std::cerr << "Unsupported statement type" << std::endl;
            return;
        }


        auto type = bound_statement->type_;

        // 3. 规划
        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(bound_statement));
        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        // 4. 确定本次使用的事务：显式事务复用会话事务，否则新建（autocommit）
        bool autocommit_stmt = !session_.InExplicitTxn();
        std::shared_ptr<Transaction> transaction;
        if (autocommit_stmt) {
            transaction = transaction_manager_->Begin();
        } else {
            transaction = session_.GetTxn();
            // READ COMMITTED：每条语句前刷新快照（RR 则整个事务复用同一快照）
            if (transaction->isolation_level_ == IsolationLevel::READ_COMMITTED) {
                transaction->read_view_ = transaction_manager_->CreateReadView(transaction->tx_id_);
            }
        }

        auto executor_context = std::make_shared<ExecutorContext>(
            catalog_, buffer_manager_, meta_page_,
            transaction_manager_, transaction, undo_log_manager_, log_manager_);

        Execute execute(executor_context);
        ExecutorFactory factory(executor_context);
        auto executor = factory.BuildExecutor(physical_operator);

        try {
            // 根据语句类型执行
            if (type == StatementType::SELECT) {
                execute.ExecuteQuery(std::move(executor));
            } else if (type == StatementType::INSERT) {
                execute.ExecuteInsert(std::move(executor));
            } else if (type == StatementType::CREATE_TABLE) {
                execute.ExecutorCreateTable(std::move(executor));
            } else if (type == StatementType::DELETE) {
                execute.ExecuteDelete(std::move(executor));
            } else if (type == StatementType::UPDATE) {
                execute.ExecuteUpdate(std::move(executor));
            } else {
                execute.ExecuteQuery(std::move(executor));
            }
            // 仅 autocommit 语句立即提交；显式事务等待 COMMIT/ROLLBACK
            if (autocommit_stmt) {
                transaction_manager_->Commit(transaction->tx_id_);
            }
        } catch (...) {
            // autocommit 语句失败直接回滚；显式事务失败也回滚并结束该事务
            transaction_manager_->Abort(transaction->tx_id_);
            if (!autocommit_stmt) {
                session_.ClearTxn();
            }
            throw;
        }
    }

    // 处理 BEGIN / COMMIT / ROLLBACK
    void HandleTransactionStatement(TxnCommand command) {
        switch (command) {
            case TxnCommand::BEGIN: {
                if (session_.InExplicitTxn()) {
                    std::cerr << "Already in a transaction" << std::endl;
                    return;
                }
                auto txn = transaction_manager_->Begin();
                txn->isolation_level_ = session_.isolation_level_;
                session_.SetTxn(txn);
                std::cout << "BEGIN" << std::endl;
                break;
            }
            case TxnCommand::COMMIT: {
                if (!session_.InExplicitTxn()) {
                    std::cerr << "No transaction in progress" << std::endl;
                    return;
                }
                transaction_manager_->Commit(session_.GetTxn()->tx_id_);
                session_.ClearTxn();
                std::cout << "COMMIT" << std::endl;
                break;
            }
            case TxnCommand::ROLLBACK: {
                if (!session_.InExplicitTxn()) {
                    std::cerr << "No transaction in progress" << std::endl;
                    return;
                }
                transaction_manager_->Abort(session_.GetTxn()->tx_id_);
                session_.ClearTxn();
                std::cout << "ROLLBACK" << std::endl;
                break;
            }
        }
    }

    void PrintHelp() {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  CREATE TABLE - Create a new table" << std::endl;
        std::cout << "  INSERT      - Insert data into a table" << std::endl;
        std::cout << "  SELECT      - Query data from tables" << std::endl;
        std::cout << "  DELETE      - Delete data from tables" << std::endl;
        std::cout << "  help/?      - Show this help message" << std::endl;
        std::cout << "  exit/quit   - Exit the terminal" << std::endl;
    }

    static void Trim(std::string &s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            s.clear();
            return;
        }
        size_t end = s.find_last_not_of(" \t\n\r");
        s = s.substr(start, end - start + 1);
    }

    std::shared_ptr<Catalog> catalog_;
    std::shared_ptr<PosixDiskManager> disk_manager_;
    std::shared_ptr<BufferManager> buffer_manager_;
    std::shared_ptr<MetaPage> meta_page_;
    std::shared_ptr<TransactionManager> transaction_manager_;
    std::shared_ptr<UndoLogManager> undo_log_manager_;
    std::shared_ptr<LogManager> log_manager_;
    ClientSession session_;
};

int main() {
    YourSQLTerminal terminal;
    terminal.Run();
    return 0;
}