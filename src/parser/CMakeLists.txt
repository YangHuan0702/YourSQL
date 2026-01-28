add_subdirectory(transform)

add_library(yoursql_parser OBJECT parser.cpp transformer.cpp)

target_link_libraries(yoursql_parser glog::glog)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_parser> PARENT_SCOPE)