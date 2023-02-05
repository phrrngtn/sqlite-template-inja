cmake_minimum_required(VERSION 3.0)

project(sqlite-template-inja)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_path(SQLITE_INCLUDE_DIR sqlite3ext.h)
include_directories(${SQLITE_INCLUDE_DIR})

find_package(inja CONFIG REQUIRED)
add_library(inja SHARED sqlite_template_inja.cpp)
# xref https://github.com/pantor/inja/issues/245
# inja needs C++ 17
set_target_properties(inja PROPERTIES CXX_STANDARD 17)