cmake_minimum_required( VERSION 2.6 )

project ( SDL_FFMPEG )

set( CMAKE_C_FLAGS "-Wall -ansi -std=c99 -O2 -D_ISOC99_SOURCE" )

if( WIN32 )

	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32" )

endif( WIN32 )

set( CMAKE_DEBUG_POSTFIX "d" )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} )

set( PROJECT_VERSION "1.0.0" )

set( DOCUMENTATION_SOURCE "${SDL_FFMPEG_SOURCE_DIR}/src ${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDL_ffmpeg.h" )

set( DOCUMENTATION_HTML_FOOTER "${SDL_FFMPEG_SOURCE_DIR}/doc/footer.html" )

add_subdirectory( lib )

add_subdirectory( bin )

add_subdirectory( doc )
