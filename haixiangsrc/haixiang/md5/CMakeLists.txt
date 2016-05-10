cmake_minimum_required(VERSION 2.6)
project(md5)

include_directories( ../dependencies/)

set(LIBRARY_OUTPUT_PATH ${md5_SOURCE_DIR}/../libs)
aux_source_directory(. src_sqlwrap)
add_library(md5 STATIC ${src_sqlwrap})
