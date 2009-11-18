# Locate SWSCALE library
# This module defines
# SWSCALE_LIBRARY, the name of the library to link against
# SWSCALE_FOUND, if false, do not try to link to SWSCALE
# SWSCALE_INCLUDE_DIR, where to find SWSCALE.h
#

set( SWSCALE_FOUND "NO" )

find_path( SWSCALE_INCLUDE_DIR swscale.h
  HINTS
  PATH_SUFFIXES include libswscale
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include 
  /opt/include
  /mingw/include
)

#message( "SWSCALE_INCLUDE_DIR is ${SWSCALE_INCLUDE_DIR}" )

find_library( SWSCALE_LIBRARY
  NAMES swscale
  HINTS
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

#message( "SWSCALE_LIBRARY is ${SWSCALE_LIBRARY}" )

set( SWSCALE_FOUND "YES" )

#message( "SWSCALE_LIBRARY is ${SWSCALE_LIBRARY}" )
