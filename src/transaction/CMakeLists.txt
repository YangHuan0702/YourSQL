add_library(yoursql_transaction OBJECT read_view.cpp transaction_manager.cpp undo_log.cpp undo_log_manager.cpp)

set(ALL_OBJECT_FILES ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:yoursql_transaction> PARENT_SCOPE)