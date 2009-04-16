add_library( 	SDL_ffmpeg
				${SDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c )

find_package( SDL )
include_directories(	${SDL_INCLUDE_DIR}
						${SDL_FFMPEG_SOURCE_DIR}/include )

if( APPLE OR LINUX )

	find_package( ZLIB )
	set( EXTRA_LIBRARIES	${EXTRA_LIBRARIES}
							${ZLIB_LIBRARY} )
							
endif( APPLE OR LINUX )

set( SDL_FFMPEG_LIBRARIES	avformat
							avcodec
							avutil
							${EXTRA_LIBRARIES}
							${SDL_LIBRARY} )

if( APPLE )

	set( SDL_FFMPEG_LIBRARIES	${SDL_FFMPEG_LIBRARIES}
								bz2 )

endif( APPLE )

target_link_libraries(	SDL_ffmpeg ${SDL_FFMPEG_LIBRARIES} )

include_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/include )

link_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/lib )
