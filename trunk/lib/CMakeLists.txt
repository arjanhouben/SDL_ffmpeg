#				SHARED

add_library( 	SDL_ffmpeg
				${LIBSDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c )

target_link_libraries(	SDL_ffmpeg
						avformat
						avcodec
						avutil
						z
						bz2
						SDL )

include_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/include )

link_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/lib )
