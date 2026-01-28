cmake_minimum_required(VERSION 3.12)
project(YourSQL VERSION 2026.01 LANGUAGES C CXX DESCRIPTION 一个热爱数据库的数据库)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_link_options("-fexceptions")
add_compile_options("-fexceptions")

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if (${CMAKE_CXX_COMPILER_VERSION} MATCHES "^12.")
        message(STATUS "you use the ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
    else ()
        message(WARNING "haloTP need using compiler clang version >= 12.0")
    endif ()
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
    message(STATUS "you use the ${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}")
else ()
    message(WARNING "Project Need using for Compile Clang Version >= 12.0")
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-unused-parameter -Wno-attributes")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
function(disable_target_warnings NAME)
    target_compile_options(${NAME} PRIVATE "-w")
endfunction()

set(PROJECT_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/src/include")
set(PROJECT_TEST_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/test/include")
set(PROJECT_THIRD_PARTY_INCLUDE_PATH
        "${PROJECT_SOURCE_DIR}/third_party/libpg_query/include"
        "${PROJECT_SOURCE_DIR}/third_party/sql-parser/src"
)

include_directories(${PROJECT_INCLUDE_PATH} ${PROJECT_TEST_INCLUDE_PATH} ${PROJECT_THIRD_PARTY_INCLUDE_PATH})
include_directories(BEFORE src)

add_subdirectory(third_party)
add_subdirectory(test)
add_subdirectory(src)
