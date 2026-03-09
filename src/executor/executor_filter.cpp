//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_filter.h"

using namespace YourSQL;

auto ExecutorFilter::Close() -> void {
    children_[0]->Close();
}

auto ExecutorFilter::Next(Tuple *tuple) -> bool {
    Tuple candidate;
    while (children_[0]->Next(&candidate)) {
        Value bl = expression_->Evaluate(candidate);
        if (!bl.GetBool()) {
            return false;
        }
    }
    tuple->Copy(candidate);
    return true;
}

auto ExecutorFilter::Open() -> void {
    children_[0]->Open();
}
