
find_package( Doxygen )

if( DOXYGEN_FOUND )

	configure_file( ${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.in
					${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.config
					@ONLY IMMEDIATE )

	add_custom_target( documentation
					   ${DOXYGEN_EXECUTABLE} ${SDL_FFMPEG_SOURCE_DIR}/doc/doxygen.config )

endif( DOXYGEN_FOUND )
