add_library(yoursql_binder OBJECT bound_select_statement.cpp bound_column_ref_expression.cpp bound_star_expression.cpp bound_table_ref_expression.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_binder> PARENT_SCOPE)
