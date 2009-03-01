#				SHARED

add_library( 	SDL_ffmpeg
				${LIBSDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c )

find_package( SDL )

include_directories( ${SDL_INCLUDE_DIR} )

target_link_libraries(	SDL_ffmpeg
						avformat
						avcodec
						avutil
						z
						${SDL_LIBRARY} )

#						bz2

include_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/include )

link_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/lib )
