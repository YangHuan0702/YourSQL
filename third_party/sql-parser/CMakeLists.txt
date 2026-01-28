cmake_minimum_required(VERSION 3.5)

project(sql_parser LANGUAGES CXX)


file(GLOB_RECURSE SQL_PARSER_ALL_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cpp")
file(GLOB_RECURSE SQL_PARSER_ALL_MAIN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(sql_parser STATIC ${SQL_PARSER_ALL_MAIN_FILES} ${SQL_PARSER_ALL_SOURCE_FILES})

target_include_directories(sql_parser
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>  # 如果是当前项目的构建
        $<INSTALL_INTERFACE:include>  # 安装路径下的 include
)
disable_target_warnings(sql_parser)