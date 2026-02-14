add_library(yoursql_catalog OBJECT catalog.cpp column_entry.cpp table_entry.cpp)
set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_catalog> PARENT_SCOPE)