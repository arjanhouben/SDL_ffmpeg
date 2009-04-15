cmake_minimum_required( VERSION 2.6 )

project ( SDL_FFMPEG )

set( CMAKE_C_FLAGS "-Wall -ansi -D_ISOC99_SOURCE -std=c99 -O2" )
set( CMAKE_DEBUG_POSTFIX "d" )

add_subdirectory( lib )

add_subdirectory( bin )
