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


    auto statement = std::make_unique<SelectStatement>();

    // select xxx
    for (auto item: *sqlStatement->selectList) {
        switch (item->type) {
            case hsql::kExprStar:
                statement->GetSelectList().push_back(std::make_unique<StarExpression>());
                break;
            case hsql::kExprColumnRef: {
                std::string column_name = item->name ? std::string(item->name) : "";
                std::string alias_name = item->alias ? std::string(item->alias) : "";
                statement->GetSelectList().push_back(std::make_unique<ColumnExpression>(column_name, alias_name));
                break;
            }
            default:
                // Clangd: Cannot jump from switch statement to this case label
                LOG(ERROR) << "Unknown expression type: " << item->type;
                break;
        }
    }

    // from xxx
    std::unique_ptr<YourTable> table = transformTableRef(sqlStatement->fromTable);
    statement->SetTable(table);

    // where
    std::unique_ptr<BaseExpression> where_expression = transformWhere(sqlStatement->whereClause);
    statement->SetWhereExpr(where_expression);

    // group by
    // auto group_by_description = sqlStatement->groupBy;
    // std::cout << "123" << std::endl;
    // limit offset
    return statement;
}
