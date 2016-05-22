# Find Qwt
# ~~~~~~~~
# Copyright (c) 2010, Tim Sutton <tim at linfiniti.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Once run this will define: 
# 
# QWT_FOUND       = system has QWT lib
# QWT_LIBRARY     = full path to the QWT library
# QWT_INCLUDE_DIR = where to find headers 
#


FIND_PATH(QWT_INCLUDE_DIR NAMES qwt.h PATH_SUFFIXES qwt PATHS
  /usr/include
  /usr/local/include
  "$ENV{QWT_INCLUDE_DIR}"
  "$ENV{LIB_DIR}/include" 
  "$ENV{INCLUDE}" 
  PATH_SUFFIXES qwt-qt4 qwt qwt5 qwt6
  )

FIND_LIBRARY(QWT_LIBRARY NAMES qwt qwt5 qwt6 qwt-qt4 qwt5-qt4 qwt6-qt4 PATHS
  /usr/lib
  /usr/local/lib
  "$ENV{QWT_LIB_DIR}"
  "$ENV{LIB_DIR}/lib" 
  "$ENV{LIB}/lib" 
  )

IF (QWT_INCLUDE_DIR AND QWT_LIBRARY)
  SET(QWT_FOUND TRUE)
ENDIF (QWT_INCLUDE_DIR AND QWT_LIBRARY)

# version
set ( _VERSION_FILE ${QWT_INCLUDE_DIR}/qwt_global.h )
if ( EXISTS ${_VERSION_FILE} )
  file ( STRINGS ${_VERSION_FILE} _VERSION_LINE REGEX "define[ ]+QWT_VERSION_STR" )
  if ( _VERSION_LINE )
    string ( REGEX REPLACE ".*define[ ]+QWT_VERSION_STR[ ]+\"(.*)\".*" "\\1" QWT_VERSION_STRING "${_VERSION_LINE}" )
    # Remove anything except digits and dots. Some version strings are of the form "6.0.0-svn" for example.
    string ( REGEX MATCH "[0-9.,]*" QWT_VERSION_STRING "${QWT_VERSION_STRING}")
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" QWT_MAJOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" QWT_MINOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" QWT_PATCH_VERSION "${QWT_VERSION_STRING}" )
  endif ()
endif ()

add_definitions( -DQWT_MAJOR_VERSION=${QWT_MAJOR_VERSION} -DQWT_MINOR_VERSION=${QWT_MINOR_VERSION} -DQWT_PATCH_VERSION=${QWT_PATCH_VERSION})

IF (QWT_FOUND)
  IF (NOT QWT_FIND_QUIETLY)
    MESSAGE(STATUS "Found Qwt: ${QWT_LIBRARY}")
    MESSAGE(STATUS "QWT_INCLUDE_DIR: ${QWT_INCLUDE_DIR}")
    MESSAGE(STATUS "QWT_VERSION_STRING: ${QWT_VERSION_STRING}")
  ENDIF (NOT QWT_FIND_QUIETLY)
ELSE (QWT_FOUND)
  IF (QWT_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Qwt")
  ENDIF (QWT_FIND_REQUIRED)
ENDIF (QWT_FOUND)
