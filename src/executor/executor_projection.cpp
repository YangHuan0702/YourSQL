//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_projection.h"

using namespace YourSQL;

auto ExecutorProjection::Close() -> void {
    children_[0]->Close();
}

auto ExecutorProjection::Next(Tuple *tuple) -> bool {
    Tuple input;
    if (!children_[0]->Next(&input)) {
        return false;
    }

    Row row(input.schema_);
    row.Deserialize(input);
    std::vector<Value> values;
    for (entry_id column_id : column_ids_) {
        values.push_back(row.values_[column_id]);
    }
    tuple->SetQueryResult(values);
    return true;
}

auto ExecutorProjection::Open() -> void {
    children_[0]->Open();
}
