if( APPLE )

	set( EXTRA_SRC	${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDLMain.m )
					
endif( APPLE )

add_executable(	example ${SDL_FFMPEG_SOURCE_DIR}/src/example.c
						${EXTRA_SRC} )

add_executable(	record	${SDL_FFMPEG_SOURCE_DIR}/src/record.c
						${EXTRA_SRC} )

add_executable(	multichannel	${SDL_FFMPEG_SOURCE_DIR}/src/multichannel.c
								${EXTRA_SRC} )

add_executable(	audioplayer	${SDL_FFMPEG_SOURCE_DIR}/src/audioplayer.c
							${EXTRA_SRC} )

add_executable(	extractaudio	${SDL_FFMPEG_SOURCE_DIR}/src/extractaudio.c
								${EXTRA_SRC} )

find_package( SDL )

include_directories( ${SDL_INCLUDE_DIR} )

target_link_libraries(	example
						SDL_ffmpeg
						${SDL_LIBRARY} )

target_link_libraries(	record
						SDL_ffmpeg
						${SDL_LIBRARY} )

target_link_libraries(	multichannel
						SDL_ffmpeg
						${SDL_LIBRARY} )

target_link_libraries(	audioplayer
						SDL_ffmpeg
						${SDL_LIBRARY} )

target_link_libraries(	extractaudio
						SDL_ffmpeg
						${SDL_LIBRARY} )

include_directories( ${SDL_FFMPEG_SOURCE_DIR}/include )
