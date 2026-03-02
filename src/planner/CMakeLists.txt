add_subdirectory(logical)

add_library(yoursql_planner OBJECT planner.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_planner> PARENT_SCOPE)
