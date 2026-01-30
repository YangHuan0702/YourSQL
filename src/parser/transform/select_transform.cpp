//
// Created by huan.yang on 2026-01-28.
//
#include <iostream>

#include "parser/transformer.h"
#include "parser/expression/column_expression.h"
#include "parser/expression/star_expression.h"
#include "sql/SelectStatement.h"
#include "glog/logging.h"

using namespace YourSQL;

auto Transformer::transformSelect(hsql::SelectStatement *sqlStatement) -> std::unique_ptr<SelectStatement> {
    if (!sqlStatement) { return nullptr; }

    // select xxx
    std::vector<std::unique_ptr<BaseExpression> > columns;
    // TODO: 这里需要将expression换成Statment用来体现语句
    for (auto item: *sqlStatement->selectList) {
        switch (item->type) {
            case hsql::kExprStar:
                columns.push_back(std::make_unique<StarExpression>());
                break;
            case hsql::kExprColumnRef: {
                std::string column_name = item->name ? std::string(item->name) : "";
                std::string alias_name = item->alias ? std::string(item->alias) : "";
                columns.push_back(std::make_unique<ColumnExpression>(column_name, alias_name));
                break;
            }
            default:
                // Clangd: Cannot jump from switch statement to this case label
                LOG(ERROR) << "Unknown expression type: " << item->type;
                break;
        }
        std::cout << item->alias << std::endl;
    }

    // from xxx
    std::unique_ptr<YourTable> table = transformTableRef(sqlStatement->fromTable);


    // where
    transformWhere(sqlStatement->whereClause);

    return nullptr;
}
