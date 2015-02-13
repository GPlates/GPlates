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


FIND_PATH(QWT_INCLUDE_DIR NAMES qwt.h PATHS
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

IF (QWT_FOUND)
  IF (NOT QWT_FIND_QUIETLY)
    MESSAGE(STATUS "Found Qwt: ${QWT_LIBRARY}")
  ENDIF (NOT QWT_FIND_QUIETLY)
ELSE (QWT_FOUND)
  IF (QWT_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Qwt")
  ENDIF (QWT_FIND_REQUIRED)
ENDIF (QWT_FOUND)
