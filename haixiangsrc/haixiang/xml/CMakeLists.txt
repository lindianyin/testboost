cmake_minimum_required(VERSION 2.6)
project(xml)

include_directories( ../dependencies/ C:\\boost_1_55_0)

set(LIBRARY_OUTPUT_PATH ${xml_SOURCE_DIR}/../libs)
aux_source_directory(. src_sqlwrap)
add_library(xml STATIC ${src_sqlwrap})
