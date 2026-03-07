//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_filter.h"

using namespace YourSQL;

auto ExecutorFilter::Close() -> void {

}

auto ExecutorFilter::Next(Tuple *tuple) -> bool {
    return false;
}

auto ExecutorFilter::Open() -> void {

}


