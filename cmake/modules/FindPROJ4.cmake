#
#
# This file is based on the "FindGDAL.cmake" file from the GPlates source tree.
#
# Locate proj4
# This module defines
# PROJ4_LIBRARY
# PROJ4_FOUND, if false, do not try to link to gdal 
# PROJ4_INCLUDE_DIR, where to find the headers
#
# $PROJ4_DIR is an environment variable that would
# correspond to the ./configure --prefix=$PROJ4_DIR
# used in building gdal.
#
# Created by Eric Wing. I'm not a gdal user, but OpenSceneGraph uses it 
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real gdal users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems 
# to happen).




#
# Try searching only CMake variable 'PROJ4_DIR' and environment variable
# 'PROJ4_DIR'.
#

SET(PROJ4_DIR_SEARCH "${PROJ4_DIR}")
IF(PROJ4_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ4_DIR_SEARCH}" PROJ4_DIR_SEARCH)
    SET(PROJ4_DIR_SEARCH ${PROJ4_DIR_SEARCH})
    
    FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
      PATHS ${GDAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ4 library"
    )
ENDIF(PROJ4_DIR_SEARCH)

SET(PROJ4_DIR_SEARCH "$ENV{PROJ4_DIR}")
IF(PROJ4_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ4_DIR_SEARCH}" PROJ4_DIR_SEARCH)
    SET(PROJ4_DIR_SEARCH ${PROJ4_DIR_SEARCH})
    
    FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
      PATHS ${PROJ4_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ4 library"
    )
ENDIF(PROJ4_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(PROJ4_DIR_SEARCH "$ENV{INCLUDE}")
IF(PROJ4_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ4_DIR_SEARCH}" PROJ4_DIR_SEARCH)
    SET(PROJ4_DIR_SEARCH ${PROJ4_DIR_SEARCH})
    
    FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
      PATHS ${PROJ4_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ4 library"
    )
ENDIF(PROJ4_DIR_SEARCH)

FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
    PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES include
    DOC "Include directory for the PROJ4 library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_PATH(PROJ4_INCLUDE_DIR proj_api.h
  PATHS
  ~/Library/Frameworks/proj.framework/Headers
  /Library/Frameworks/proj.framework/Headers
  /usr/local/include/proj
  /usr/local/include/PROJ
  /usr/local/include
  /usr/include/proj
  /usr/include/PROj
  /usr/include
  /sw/include/proj
  /sw/include/PROJ 
  /sw/include # Fink
  /opt/local/include/proj
  /opt/local/include/PROJ
  /opt/local/lib/proj49/include
  /opt/local/lib/PROJ49/include
  /opt/local/include # DarwinPorts
  /opt/csw/include/proj
  /opt/csw/include/PROJ
  /opt/csw/include # Blastwave
  /opt/include/proj
  /opt/include/PROJ
  /opt/include
  DOC "PROJ4 library"
)

#
# Try only searching directories of CMake variable 'PROJ4_DIR' and environment
# variable 'PROJ4_DIR'.
#

set(PROJ4_DIR_SEARCH ${PROJ4_DIR})
IF(PROJ4_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ4_DIR_SEARCH}" PROJ4_DIR_SEARCH)
    SET(PROJ4_DIR_SEARCH ${PROJ4_DIR_SEARCH})
    
    FIND_LIBRARY(PROJ4_LIBRARY
      NAMES proj proj_i
      PATHS ${PROJ4_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "PROJ4 library"
    )
ENDIF(PROJ4_DIR_SEARCH)

set(PROJ4_DIR_SEARCH $ENV{PROJ4_DIR})
IF(PROJ4_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ4_DIR_SEARCH}" PROJ4_DIR_SEARCH)
    SET(PROJ4_DIR_SEARCH ${PROJ4_DIR_SEARCH})
    
    FIND_LIBRARY(PROJ4_LIBRARY
      NAMES proj proj_i
      PATHS ${PROJ4_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "PROJ4 library"
    )
ENDIF(PROJ4_DIR_SEARCH)

FIND_LIBRARY(PROJ4_LIBRARY 
  NAMES proj proj_i
  PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib64 lib
    DOC "PROJ4 library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_LIBRARY(PROJ4_LIBRARY 
  NAMES proj proj_i
  PATHS
    "${PROJ4_INCLUDE_DIR}/.." # this only really needed on windows platforms
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/local/lib/proj49/lib
    /opt/local/lib/PROJ49/lib
    /opt/csw
    /opt
    /usr/freeware
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;PROJ4_ROOT]/lib
  PATH_SUFFIXES lib64 lib
  DOC "PROJ4 library"
)

# Found PROJ4 only if both include directory and library found.
SET(PROJ4_FOUND "NO")
IF(PROJ4_LIBRARY AND PROJ4_INCLUDE_DIR)
  SET(PROJ4_FOUND "YES")
ENDIF(PROJ4_LIBRARY AND PROJ4_INCLUDE_DIR)



