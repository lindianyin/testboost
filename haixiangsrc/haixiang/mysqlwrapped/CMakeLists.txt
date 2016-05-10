cmake_minimum_required(VERSION 2.8)
project(mysqlwrapped)

SET(CMAKE_SUPPRESS_REGENERATION 1)
SET(CMAKE_USE_RELATIVE_PATHS ON)


include_directories( ../dependencies/ ../dependencies/mysql_wrap_include C:\\boost_1_55_0 ../dependencies/mysql_win_include)

set(LIBRARY_OUTPUT_PATH ${mysqlwrapped_SOURCE_DIR}/../libs)
aux_source_directory(. src_sqlwrap)
add_library(mysqlwrapped STATIC ${src_sqlwrap})
