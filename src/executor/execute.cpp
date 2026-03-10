//
// Created by huan.yang on 2026-03-06.
//
#include "executor/execute.h"

#include <iostream>

using namespace YourSQL;


void Execute::ExecuteQuery(std::unique_ptr<Executor> root) {
    root->Open();

    Tuple tuple;
    while (root->Next(&tuple)) {
        // PrintTuple(tuple);
    }
    root->Close();
}


auto Execute::ExecuteInsert(std::unique_ptr<Executor> root) -> void {
    root->Open();
    Tuple tuple;
    root->Next(&tuple);
    root->Close();

    std::cout<<"执行成功了哥们！"<< std::endl;
}

