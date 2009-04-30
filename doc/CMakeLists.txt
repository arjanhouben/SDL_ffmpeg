
find_package( doxygen )

if( DOXYGEN_FOUND )

	add_custom_target( doc ${DOXYGEN_EXECUTABLE} ${DOXY_CONFIG} )

endif( DOXYGEN_FOUND )