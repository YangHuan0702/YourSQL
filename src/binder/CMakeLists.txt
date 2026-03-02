add_library(yoursql_binder OBJECT bound_select_statement.cpp bound_column_ref_expression.cpp bound_star_expression.cpp
        bound_table_ref_expression.cpp bound_comp_expression.cpp bound_is_null_expression.cpp bound_in_expression.cpp bound_like_expression.cpp
        bound_logical_comp_expression.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_binder> PARENT_SCOPE)
