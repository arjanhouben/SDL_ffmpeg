cmake_minimum_required( VERSION 2.6 )

project ( LIBSDL_FFMPEG )

set( CMAKE_C_FLAGS "-O2" )

add_subdirectory( lib )

add_subdirectory( bin )
