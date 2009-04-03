#				SHARED

add_library( 	SDL_ffmpeg
				${SDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c )

find_package( SDL )
include_directories(	${SDL_INCLUDE_DIR}
						${SDL_FFMPEG_SOURCE_DIR}/include )

find_package( ZLIB )

#find_package( BZ2 )

target_link_libraries(	SDL_ffmpeg
						avformat
						avcodec
						avutil
						${ZLIB_LIBRARY}
						bz2
						${SDL_LIBRARY} )

include_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/include )

link_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/lib )
