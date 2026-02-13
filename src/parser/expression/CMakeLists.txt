add_library(yoursql_parser_expression OBJECT column_expression.cpp operator_expression.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_parser_expression> PARENT_SCOPE)
