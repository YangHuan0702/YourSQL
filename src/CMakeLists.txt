add_subdirectory(parser)
add_subdirectory(catalog)
add_subdirectory(binder)

add_library(YourDB SHARED ${ALL_OBJECT_FILES})

find_package(Threads REQUIRED)
set(THIRD_PARTY_LIBS sql_parser glog::glog Threads::Threads)
target_link_libraries(YourDB ${THIRD_PARTY_LIBS})

target_include_directories(YourDB PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)