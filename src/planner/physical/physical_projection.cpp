//
// Created by huan.yang on 2026-03-05.
//
#include "planner/physical/physical_projection.h"

using namespace YourSQL;

auto PhysicalProjection::Close() -> void {

}

auto PhysicalProjection::Next() -> bool {
    return false;
}

auto PhysicalProjection::Open() -> void {

}

auto PhysicalProjection::to_string() -> std::string {
    return "";
}



