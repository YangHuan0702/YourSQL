//
// Created by huan.yang on 2026-03-04.
//
#include "planner/physical/physical_seq_scan.h"
using namespace YourSQL;

auto PhysicalSeqScan::to_string() -> std::string {
    return "SeqScan[TableId:" + std::to_string(table_id_) + "]";
}

auto PhysicalSeqScan::Close() -> void {

}

auto PhysicalSeqScan::Next() -> bool {
    return false;
}

auto PhysicalSeqScan::Open() -> void {

}




