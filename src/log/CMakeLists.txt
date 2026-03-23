add_library(yoursql_log OBJECT log_buffer.cpp log_manager.cpp log_record.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_log> PARENT_SCOPE)