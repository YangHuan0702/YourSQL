add_library(yoursql_storage_page OBJECT table_page.cpp row.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_storage_page> PARENT_SCOPE)