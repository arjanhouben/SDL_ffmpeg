set( SDLLIB ${SDL_LIBRARY} )

if( APPLE )

	set( EXTRA_SRC	${SDL_FFMPEG_SOURCE_DIR}/include/SDL/SDLMain.m )
					
endif( APPLE )


if( WIN32 )

    set( SDLLIB ${SDLLIB}
                ${SDLMAIN_LIBRARY}  )

endif( WIN32 )


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

include_directories( ${SDL_INCLUDE_DIR}
					 ${SDL_FFMPEG_SOURCE_DIR}/include/SDL )

target_link_libraries(	example
						SDL_ffmpeg
						${SDLLIB} )

target_link_libraries(	record
						SDL_ffmpeg
						${SDLLIB} )

target_link_libraries(	multichannel
						SDL_ffmpeg
						${SDLLIB} )

target_link_libraries(	audioplayer
						SDL_ffmpeg
						${SDLLIB} )

target_link_libraries(	extractaudio
						SDL_ffmpeg
						${SDLLIB} )

include_directories( ${SDL_FFMPEG_SOURCE_DIR}/include )
