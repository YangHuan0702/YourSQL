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

using namespace YourSQL;

class YourSQLTerminal {
public:
    YourSQLTerminal() {
        // 初始化各个组件
        catalog_ = std::make_shared<Catalog>();
        disk_manager_ = std::make_shared<PosixDiskManager>();
        buffer_manager_ = std::make_shared<BufferManager>(disk_manager_);
        meta_page_ = std::make_shared<MetaPage>(buffer_manager_);

        executor_context_ = std::make_shared<ExecutorContext>(
            catalog_, buffer_manager_, meta_page_, nullptr, nullptr, nullptr);
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
        }
        // else if (auto *delete_stmt = dynamic_cast<DeleteStatement*>(statement.get())) {
        //     statement.release();
        //     auto useStatement = std::unique_ptr<DeleteStatement>(delete_stmt);
        //     auto bound_delete = binder.BoundDeleteStatement(std::move(useStatement));
        //     bound_statement = std::unique_ptr<BoundStatement>(bound_delete.release());
        // }
        else {
            std::cerr << "Unsupported statement type" << std::endl;
            return;
        }


        auto type = bound_statement->type_;

        // 3. 规划
        Planner planner;
        auto logical_operator = planner.CreateLogicalPlan(std::move(bound_statement));
        auto physical_operator = planner.CreatePhysicalPlan(logical_operator);

        // 4. 执行
        Execute execute(executor_context_);
        ExecutorFactory factory(executor_context_); // table_id 需要从计划中获取
        auto executor = factory.BuildExecutor(physical_operator);

        // 根据语句类型执行
        if (type == StatementType::SELECT) {
            execute.ExecuteQuery(std::move(executor));
        } else if (type == StatementType::INSERT) {
            execute.ExecuteInsert(std::move(executor));
        } else if (type == StatementType::CREATE_TABLE) {
            execute.ExecutorCreateTable(std::move(executor));
        }
        // else if (physical_operator->type_ == PhysicalOperatorTypes::DELETE) {
        //     execute.ExecuteQuery(std::move(executor));
        // }
        else {
            execute.ExecuteQuery(std::move(executor));
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
    std::shared_ptr<ExecutorContext> executor_context_;
};

int main() {
    YourSQLTerminal terminal;
    terminal.Run();
    return 0;
}