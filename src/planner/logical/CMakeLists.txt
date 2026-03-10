add_library(yoursql_planner_logical OBJECT logical_select_plan.cpp logical_insert_plan.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_planner_logical> PARENT_SCOPE)