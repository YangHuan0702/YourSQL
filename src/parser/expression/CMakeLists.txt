add_library(yoursql_parser_expression OBJECT column_expression.cpp operator_expression.cpp in_expression.cpp is_null_expression.cpp like_expression.cpp logic_expression.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_parser_expression> PARENT_SCOPE)
