add_library(yoursql_parser_table_ref OBJECT transform_table.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_parser_table_ref> PARENT_SCOPE)
