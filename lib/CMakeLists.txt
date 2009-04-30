if( NOT APPLE )

	set( LIBTYPE "SHARED" )

endif( NOT APPLE )

add_library( 	SDL_ffmpeg
				${LIBTYPE}
				${SDL_FFMPEG_SOURCE_DIR}/src/SDL_ffmpeg.c )

if( WIN32 )

	set_target_properties( SDL_ffmpeg PROPERTIES PREFIX "" )

endif( WIN32 )

find_package( SDL )
find_package( avformat )
find_package( avcodec )
find_package( avutil )

include_directories(	${SDL_INCLUDE_DIR}
						${AVFORMAT_INCLUDE_DIR}
						${AVCODEC_INCLUDE_DIR}
						${AVUTIL_INCLUDE_DIR}
						${SDL_FFMPEG_SOURCE_DIR}/include/SDL )

if( UNIX )

	find_package( ZLIB )
	set( EXTRA_LIBRARIES	${EXTRA_LIBRARIES} )
	set( EXTRA_LIBRARIES	${EXTRA_LIBRARIES} ${ZLIB_LIBRARY} )

endif( UNIX )

set( SDL_FFMPEG_LIBRARIES	${AVFORMAT_LIBRARY}
							${AVCODEC_LIBRARY}
							${AVUTIL_LIBRARY}
							${EXTRA_LIBRARIES}
							${SDL_LIBRARY} )

if( APPLE )

	set( SDL_FFMPEG_LIBRARIES	${SDL_FFMPEG_LIBRARIES}
								bz2 )

endif( APPLE )

target_link_libraries(	SDL_ffmpeg
						${SDL_FFMPEG_LIBRARIES} )

include_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/include )

link_directories( ${LIBSDL_FFMPEG_SOURCE_DIR}/lib )
