cmake_minimum_required( VERSION 2.6 )

project( EXAMPLES )

add_executable(	example         ${EXAMPLES_SOURCE_DIR}/example.c )

add_executable(	record          ${EXAMPLES_SOURCE_DIR}/record.c )

add_executable(	multichannel	${EXAMPLES_SOURCE_DIR}/multichannel.c )

add_executable(	audioplayer     ${EXAMPLES_SOURCE_DIR}/audioplayer.c )

add_executable(	extractaudio	${EXAMPLES_SOURCE_DIR}/extractaudio.c )

find_package( SDL REQUIRED )

find_package( SDL_ffmpeg REQUIRED )

include_directories( ${SDL_INCLUDE_DIR} ${SDL_FFMPEG_INCLUDE_DIR} )

target_link_libraries(	example
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	record
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	multichannel
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	audioplayer
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

target_link_libraries(	extractaudio
						${SDL_FFMPEG_LIBRARY}
						${SDL_LIBRARY} )

include_directories( ${SDL_FFMPEG_SOURCE_DIR}/include )
