set( EXAMPLE_SRC ${SDL_FFMPEG_SOURCE_DIR}/src/example.c )

set( RECORD_SRC ${SDL_FFMPEG_SOURCE_DIR}/src/record.c )

if( APPLE )

	set( EXAMPLE_SRC ${EXAMPLE_SRC}
					 ${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDLMain.m )

	set( RECORD_SRC ${RECORD_SRC}
					 ${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDLMain.m )
					
endif( APPLE )

add_executable(	example ${EXAMPLE_SRC} )

add_executable(	record ${RECORD_SRC} )

find_package( SDL )

include_directories( ${SDL_INCLUDE_DIR} )

target_link_libraries(	example
						SDL_ffmpeg
						${SDL_LIBRARY} )

target_link_libraries(	record
						SDL_ffmpeg
						${SDL_LIBRARY} )

include_directories( ${SDL_FFMPEG_SOURCE_DIR}/include )
