//
// Created by huan.yang on 2026-03-06.
//
#include "executor/execute.h"

#include <iostream>
#include <iomanip>
#include "storage/page/tuple.h"

using namespace YourSQL;

auto Execute::PrintTuple(const Tuple &tuple) -> void {
    // 第一次打印时，输出表头
    if (!header_printed_) {
        current_schema_ = tuple.schema_;
        std::vector<size_t> column_widths;

        // 计算每列的宽度（取列名和数据最大宽度）
        for (const auto &column : tuple.schema_.columns_) {
            size_t width = column.name_.length();
            column_widths.push_back(width);
        }

        // 打印表头
        std::cout << "+";
        for (size_t width : column_widths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;

        std::cout << "|";
        for (size_t i = 0; i < tuple.schema_.columns_.size(); ++i) {
            const auto &column = tuple.schema_.columns_[i];
            std::cout << " " << std::left << std::setw(column_widths[i]) << column.name_ << " |";
        }
        std::cout << std::endl;

        std::cout << "+";
        for (size_t width : column_widths) {
            std::cout << std::string(width + 2, '-') << "+";
        }
        std::cout << std::endl;

        header_printed_ = true;
    }

    // 打印数据行
    std::cout << "|";
    for (size_t i = 0; i < tuple.query_result_.size(); ++i) {
        const auto &value = tuple.query_result_[i];
        const auto &column = tuple.schema_.columns_[i];
        size_t width = column.name_.length();

        std::cout << " ";

        if (value.IsNull()) {
            std::cout << std::left << std::setw(width) << "NULL";
        } else {
            switch (column.column_types) {
                case ColumnTypes::INTEGER:
                    std::cout << std::left << std::setw(width) << value.GetInt();
                    break;
                case ColumnTypes::DOUBLE:
                    std::cout << std::left << std::setw(width) << value.GetDouble();
                    break;
                case ColumnTypes::VARCHAR:
                case ColumnTypes::VARCHAR2:
                    std::cout << std::left << std::setw(width) << value.GetString();
                    break;
                case ColumnTypes::BOOL:
                    std::cout << std::left << std::setw(width) << (value.GetBool() ? "true" : "false");
                    break;
                case ColumnTypes::TIMESTAMP:
                    std::cout << std::left << std::setw(width) << value.GetTimestamp();
                    break;
                default:
                    std::cout << std::left << std::setw(width) << "UNKNOWN";
                    break;
            }
        }

        std::cout << " |";
    }
    std::cout << std::endl;

    row_count_++;
}


void Execute::ExecuteQuery(std::unique_ptr<Executor> root) {
    root->Open();

    Tuple tuple;
    row_count_ = 0;
    header_printed_ = false;

    while (root->Next(&tuple)) {
        PrintTuple(tuple);
    }
    root->Close();

    // 如果有结果，打印表格底部边界
    if (header_printed_ && row_count_ > 0) {
        std::cout << "+";
        for (const auto &column : current_schema_.columns_) {
            std::cout << std::string(column.name_.length() + 2, '-') << "+";
        }
        std::cout << std::endl;

        // 打印结果数量
        std::cout << row_count_ << " row(s) in set" << std::endl;
    }
}


auto Execute::ExecuteInsert(std::unique_ptr<Executor> root) -> void {
    root->Open();
    Tuple tuple;
    root->Next(&tuple);
    root->Close();

    std::cout<<"执行成功了哥们！"<< std::endl;
}

