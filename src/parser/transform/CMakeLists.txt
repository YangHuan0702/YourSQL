add_library(yoursql_parser_transform OBJECT select_transform.cpp)

target_link_libraries(yoursql_parser_transform glog::glog)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_parser_transform> PARENT_SCOPE)
