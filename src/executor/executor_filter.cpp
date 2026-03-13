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
        if (bl.GetBool()) {  // 如果条件满足，返回这个 tuple
            tuple->Copy(candidate);
            return true;
        }
    }
    return false;
}

auto ExecutorFilter::Open() -> void {
    children_[0]->Open();
}
