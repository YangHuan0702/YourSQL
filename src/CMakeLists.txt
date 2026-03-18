add_subdirectory(parser)
add_subdirectory(catalog)
add_subdirectory(binder)
add_subdirectory(planner)
add_subdirectory(storage)
add_subdirectory(buffer)
add_subdirectory(executor)
add_subdirectory(index)

add_library(YourSQL SHARED ${ALL_OBJECT_FILES})

find_package(Threads REQUIRED)
set(THIRD_PARTY_LIBS sql_parser glog::glog Threads::Threads benchmark::benchmark)
target_link_libraries(YourSQL ${THIRD_PARTY_LIBS})

target_include_directories(YourSQL PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)