#
#
# This file is based on the "FindGDAL.cmake" file from the GPlates source tree.
#
# Locate proj
# This module defines
# PROJ_LIBRARY
# PROJ_FOUND, if false, do not try to link to gdal 
# PROJ_INCLUDE_DIR, where to find the headers
#
# $PROJ_DIR is an environment variable that would
# correspond to the ./configure --prefix=$PROJ_DIR
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
# Try searching only CMake variable 'PROJ_DIR' and environment variable 'PROJ_DIR'.
#

SET(PROJ_DIR_SEARCH "${PROJ_DIR}")
IF(PROJ_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ_DIR_SEARCH}" PROJ_DIR_SEARCH)
    SET(PROJ_DIR_SEARCH ${PROJ_DIR_SEARCH})
    
    FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
      PATHS ${GDAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ library"
    )
ENDIF(PROJ_DIR_SEARCH)

SET(PROJ_DIR_SEARCH "$ENV{PROJ_DIR}")
IF(PROJ_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ_DIR_SEARCH}" PROJ_DIR_SEARCH)
    SET(PROJ_DIR_SEARCH ${PROJ_DIR_SEARCH})
    
    FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
      PATHS ${PROJ_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ library"
    )
ENDIF(PROJ_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(PROJ_DIR_SEARCH "$ENV{INCLUDE}")
IF(PROJ_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ_DIR_SEARCH}" PROJ_DIR_SEARCH)
    SET(PROJ_DIR_SEARCH ${PROJ_DIR_SEARCH})
    
    FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
      PATHS ${PROJ_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the PROJ library"
    )
ENDIF(PROJ_DIR_SEARCH)

FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
    PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES include
    DOC "Include directory for the PROJ library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_PATH(PROJ_INCLUDE_DIR proj_api.h
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
  DOC "Include directory for the PROJ library"
)

#
# Try only searching directories of CMake variable 'PROJ_DIR' and environment variable 'PROJ_DIR'.
#

set(PROJ_DIR_SEARCH ${PROJ_DIR})
IF(PROJ_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ_DIR_SEARCH}" PROJ_DIR_SEARCH)
    SET(PROJ_DIR_SEARCH ${PROJ_DIR_SEARCH})
    
    FIND_LIBRARY(PROJ_LIBRARY
      NAMES proj proj_i proj_6_0 proj_6_0 proj_6_1 proj_6_2 proj_6_3
      PATHS ${PROJ_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "PROJ library"
    )
ENDIF(PROJ_DIR_SEARCH)

set(PROJ_DIR_SEARCH $ENV{PROJ_DIR})
IF(PROJ_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${PROJ_DIR_SEARCH}" PROJ_DIR_SEARCH)
    SET(PROJ_DIR_SEARCH ${PROJ_DIR_SEARCH})
    
    FIND_LIBRARY(PROJ_LIBRARY
      NAMES proj proj_i proj_6_0 proj_6_0 proj_6_1 proj_6_2 proj_6_3
      PATHS ${PROJ_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "PROJ library"
    )
ENDIF(PROJ_DIR_SEARCH)

FIND_LIBRARY(PROJ_LIBRARY 
  NAMES proj proj_i proj_6_0 proj_6_0 proj_6_1 proj_6_2 proj_6_3
  PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib64 lib
    DOC "PROJ library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_LIBRARY(PROJ_LIBRARY 
  NAMES proj proj_i proj_6_0 proj_6_0 proj_6_1 proj_6_2 proj_6_3
  PATHS
    "${PROJ_INCLUDE_DIR}/.." # this only really needed on windows platforms
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
  DOC "PROJ library"
)

# Found PROJ only if both include directory and library found.
SET(PROJ_FOUND "NO")
IF(PROJ_LIBRARY AND PROJ_INCLUDE_DIR)
  SET(PROJ_FOUND "YES")
ENDIF(PROJ_LIBRARY AND PROJ_INCLUDE_DIR)



