add_library(yoursql_buffer OBJECT buffer_manager.cpp meta_page.cpp table_iterator.cpp)

target_link_libraries(yoursql_buffer glog::glog)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_buffer> PARENT_SCOPE)