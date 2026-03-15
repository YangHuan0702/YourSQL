add_library(yoursql_index OBJECT b_plus_index_manager.cpp b_plus_index_iterator.cpp b_plus_internal_page.cpp b_plus_leaf_page.cpp index_page.cpp)
target_link_libraries(yoursql_index glog::glog)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_index> PARENT_SCOPE)