cmake_minimum_required( VERSION 2.6 )

project ( SDL_FFMPEG )

#set( CMAKE_C_FLAGS "-O2" )
#set( CMAKE_C_FLAGS "-ggdb" )
#set( CMAKE_DEBUG_POSTFIX "d" )

add_subdirectory( lib )

add_subdirectory( bin )
