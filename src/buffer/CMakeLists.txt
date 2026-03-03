add_library(yoursql_buffer OBJECT buffer_manager.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_buffer> PARENT_SCOPE)