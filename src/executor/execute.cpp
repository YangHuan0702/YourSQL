//
// Created by huan.yang on 2026-03-06.
//
#include "executor/execute.h"

using namespace YourSQL;


void Execute::ExecuteQuery(std::unique_ptr<Executor> root) {
    root->Open();

    Tuple tuple;
    while (root->Next(&tuple)) {
        // PrintTuple(tuple);
    }
    root->Close();
}

