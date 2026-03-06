//
// Created by huan.yang on 2026-03-05.
//
#include "planner/physical/physical_filter.h"
using namespace YourSQL;

auto PhysicalFilter::Close() -> void {

}

auto PhysicalFilter::Next() -> bool {
    return false;
}

auto PhysicalFilter::Open() -> void {

}

auto PhysicalFilter::to_string() -> std::string {
    return "";
}



