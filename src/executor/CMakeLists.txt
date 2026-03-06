add_library(yoursql_executor OBJECT executor_factory.cpp executor_projection.cpp executor_seq_scan.cpp executor_filter.cpp execute.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_executor> PARENT_SCOPE)