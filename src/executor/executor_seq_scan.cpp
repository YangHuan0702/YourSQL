//
// Created by huan.yang on 2026-03-06.
//
#include "executor/executor_seq_scan.h"

using namespace YourSQL;

auto ExecutorSeqScan::Close() -> void {

}

auto ExecutorSeqScan::Next(Tuple *tuple) -> bool {
    return false;
}

auto ExecutorSeqScan::Open() -> void {
    cursor_ = 0;
}


