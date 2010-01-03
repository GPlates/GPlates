# Locate cgal
# This module defines
# CGAL_LIBRARY
# CGAL_FOUND, if false, do not try to link to cgal 
# CGAL_INCLUDE_DIR, where to find the headers
#
# $CGALDIR is an environment variable that would
# correspond to the ./configure --prefix=$CGAL_DIR
# used in building cgal.
#
# Created by Eric Wing. I'm not a cgal user, but OpenSceneGraph uses it 
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real cgal users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems 
# to happen).

# This makes the presumption that you are include cgal.h like
# #include "cgal.h"



#
# Try searching only CMake variable 'CGAL_DIR' and environment variable
# 'CGAL_DIR'.
#

SET(CGAL_DIR_SEARCH "${CGAL_DIR}")
IF(CGAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${CGAL_DIR_SEARCH}" CGAL_DIR_SEARCH)
    SET(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH})
    
    FIND_PATH(CGAL_INCLUDE_DIR CGAL/CORE/CORE.h
      PATHS ${CGAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the CGAL library"
    )
ENDIF(CGAL_DIR_SEARCH)

SET(CGAL_DIR_SEARCH "$ENV{CGAL_DIR}")
IF(CGAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${CGAL_DIR_SEARCH}" CGAL_DIR_SEARCH)
    SET(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH})
    
    FIND_PATH(CGAL_INCLUDE_DIR CGAL/CORE/CORE.h
      PATHS ${CGAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the CGAL library"
    )
ENDIF(CGAL_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(CGAL_DIR_SEARCH "$ENV{INCLUDE}")
IF(CGAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${CGAL_DIR_SEARCH}" CGAL_DIR_SEARCH)
    SET(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH})
    
    FIND_PATH(CGAL_INCLUDE_DIR CGAL/CORE/CORE.h
      PATHS ${CGAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the CGAL library"
    )
ENDIF(CGAL_DIR_SEARCH)

FIND_PATH(CGAL_INCLUDE_DIR CGAL/CORE/CORE.h
    PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES include
    DOC "Include directory for the CGAL library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_PATH(CGAL_INCLUDE_DIR CGAL/CORE/CORE.h
  PATHS
  ~/Library/Frameworks/cgal.framework/Headers
  /Library/Frameworks/cgal.framework/Headers
  /usr/local/include/cgal
  /usr/local/include/CGAL
  /usr/local/include
  /usr/include/cgal
  /usr/include/CGAL
  /usr/include
  /sw/include/cgal 
  /sw/include/CGAL 
  /sw/include # Fink
  /opt/local/include/cgal
  /opt/local/include/CGAL
  /opt/local/include # DarwinPorts
  /opt/csw/include/cgal
  /opt/csw/include/CGAL
  /opt/csw/include # Blastwave
  /opt/include/cgal
  /opt/include/CGAL
  /opt/include
  DOC "Include directory for the CGAL library"
)

#
# The various possible library names for the CGAL library are listed here.
# 
set(CGAL_LIBRARY_NAMES cgal cgal_i cgal1.5.2 cgal1.5.0 cgal1.4.0 cgal1.3.2 CGAL)

#
# Try only searching directories of CMake variable 'CGAL_DIR' and environment
# variable 'CGAL_DIR'.
#

set(CGAL_DIR_SEARCH ${CGAL_DIR})
IF(CGAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${CGAL_DIR_SEARCH}" CGAL_DIR_SEARCH)
    SET(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH})
    
    # cgal1.3.2 on Ubuntu 7.04 in 2007-2008 (GPlates changeset 2107).
    # cgal1.4.0 on Ubuntu 7.10 in 2008 (GPlates changeset 2222).
    # cgal1.5.0 on Debian testing in 2008 (GPlates changeset 2773).
    # cgal1.5.2 on OpenSUSE 11 in 2008 (GPlates changeset 3954).
    FIND_LIBRARY(CGAL_LIBRARY
      NAMES ${CGAL_LIBRARY_NAMES}
      PATHS ${CGAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "CGAL library"
    )
ENDIF(CGAL_DIR_SEARCH)

set(CGAL_DIR_SEARCH $ENV{CGAL_DIR})
IF(CGAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${CGAL_DIR_SEARCH}" CGAL_DIR_SEARCH)
    SET(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH})
    
    FIND_LIBRARY(CGAL_LIBRARY
      NAMES ${CGAL_LIBRARY_NAMES}
      PATHS ${CGAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "CGAL library"
    )
ENDIF(CGAL_DIR_SEARCH)

FIND_LIBRARY(CGAL_LIBRARY 
  NAMES ${CGAL_LIBRARY_NAMES}
  PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib64 lib
    DOC "CGAL library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_LIBRARY(CGAL_LIBRARY 
  NAMES ${CGAL_LIBRARY_NAMES}
  PATHS
    "${CGAL_INCLUDE_DIR}/.." # this only really needed on windows platforms
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
    /usr/freeware
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;CGAL_ROOT]/lib
  PATH_SUFFIXES lib64 lib
  DOC "CGAL library"
)

# Found CGAL only if both include directory and library found.
SET(CGAL_FOUND "NO")
IF(CGAL_LIBRARY AND CGAL_INCLUDE_DIR)
  SET(CGAL_FOUND "YES")
ENDIF(CGAL_LIBRARY AND CGAL_INCLUDE_DIR)


