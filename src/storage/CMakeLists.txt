add_subdirectory(page)


add_library(yoursql_storage OBJECT posix_disk_manager.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_storage> PARENT_SCOPE)
