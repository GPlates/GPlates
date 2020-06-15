# Locate gdal
# This module defines
# GDAL_LIBRARY
# GDAL_FOUND, if false, do not try to link to gdal 
# GDAL_INCLUDE_DIR, where to find the headers
#
# $GDALDIR is an environment variable that would
# correspond to the ./configure --prefix=$GDAL_DIR
# used in building gdal.
#
# Created by Eric Wing. I'm not a gdal user, but OpenSceneGraph uses it 
# for osgTerrain so I whipped this module together for completeness.
# I actually don't know the conventions or where files are typically
# placed in distros.
# Any real gdal users are encouraged to correct this (but please don't
# break the OS X framework stuff when doing so which is what usually seems 
# to happen).

# This makes the presumption that you are include gdal.h like
# #include "gdal.h"



#
# Try searching only CMake variable 'GDAL_DIR' and environment variable
# 'GDAL_DIR'.
#

FUNCTION(FIND_OTHER_GDAL_INCLUDE_DIRS _GDAL_DIR_SEARCH)
IF(WIN32)
  FIND_PATH(GDAL_OGR_INCLUDE_DIR ogr_core.h
		PATHS 
		${_GDAL_DIR_SEARCH}
		"${_GDAL_DIR_SEARCH}/ogr"
		NO_DEFAULT_PATH
		PATH_SUFFIXES include
		DOC "Include directory for the GDAL ogr library"
		)
		
		FIND_PATH(GDAL_PORT_INCLUDE_DIR cpl_port.h
		PATHS 
		${_GDAL_DIR_SEARCH}
		"${_GDAL_DIR_SEARCH}/port"
		NO_DEFAULT_PATH
		PATH_SUFFIXES include
		DOC "Include directory for the GDAL port library"
		)
		
		FIND_PATH(GDAL_FRMTS_INCLUDE_DIR ogrsf_frmts.h
		PATHS 
		${_GDAL_DIR_SEARCH}
		"${_GDAL_DIR_SEARCH}/ogr/ogrsf_frmts"
		NO_DEFAULT_PATH
		PATH_SUFFIXES include
		DOC "Include directory for the GDAL ogrsf_frmts library"
		)
ENDIF(WIN32)		
ENDFUNCTION()

SET(GDAL_DIR_SEARCH "${GDAL_DIR}")
IF(GDAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GDAL_DIR_SEARCH}" GDAL_DIR_SEARCH)
    SET(GDAL_DIR_SEARCH ${GDAL_DIR_SEARCH})
    
    FIND_PATH(GDAL_INCLUDE_DIR gdal.h
      PATHS 
		${GDAL_DIR_SEARCH}
		"${GDAL_DIR_SEARCH}/gcore"
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GDAL library"
    )
	FIND_OTHER_GDAL_INCLUDE_DIRS(${GDAL_DIR_SEARCH})
ENDIF(GDAL_DIR_SEARCH)

SET(GDAL_DIR_SEARCH "$ENV{GDAL_DIR}")
IF(GDAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GDAL_DIR_SEARCH}" GDAL_DIR_SEARCH)
    SET(GDAL_DIR_SEARCH ${GDAL_DIR_SEARCH})
    
    FIND_PATH(GDAL_INCLUDE_DIR gdal.h
      PATHS 
	  ${GDAL_DIR_SEARCH}
	  "${GDAL_DIR_SEARCH}/gcore"
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GDAL library"
    )
	FIND_OTHER_GDAL_INCLUDE_DIRS(${GDAL_DIR_SEARCH})
	
ENDIF(GDAL_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(GDAL_DIR_SEARCH "$ENV{INCLUDE}")
IF(GDAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GDAL_DIR_SEARCH}" GDAL_DIR_SEARCH)
    SET(GDAL_DIR_SEARCH ${GDAL_DIR_SEARCH})
    
    FIND_PATH(GDAL_INCLUDE_DIR gdal.h
      PATHS ${GDAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GDAL library"
    )
	FIND_OTHER_GDAL_INCLUDE_DIRS(${GDAL_DIR_SEARCH})
ENDIF(GDAL_DIR_SEARCH)

SET(GDAL_DIR_SEARCH "${CMAKE_PREFIX_PATH}")
IF(GDAL_DIR_SEARCH)
	FIND_PATH(GDAL_INCLUDE_DIR gdal.h
		PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
		NO_DEFAULT_PATH
		PATH_SUFFIXES include
		DOC "Include directory for the GDAL library"
	)
	FIND_OTHER_GDAL_INCLUDE_DIRS(${GDAL_DIR_SEARCH})
ENDIF(GDAL_DIR_SEARCH)	

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_PATH(GDAL_INCLUDE_DIR gdal.h
  PATHS
  ~/Library/Frameworks/gdal.framework/Headers
  /Library/Frameworks/gdal.framework/Headers
  /usr/local/include/gdal
  /usr/local/include/GDAL
  /usr/local/include
  /usr/include/gdal
  /usr/include/GDAL
  /usr/include
  /sw/include/gdal 
  /sw/include/GDAL 
  /sw/include # Fink
  /opt/local/include/gdal
  /opt/local/include/GDAL
  /opt/local/include # DarwinPorts
  /opt/csw/include/gdal
  /opt/csw/include/GDAL
  /opt/csw/include # Blastwave
  /opt/include/gdal
  /opt/include/GDAL
  /opt/include
  DOC "Include directory for the GDAL library"
)

#
# The various possible library names for the GDAL library are listed here.
# 
set(GDAL_LIBRARY_NAMES
	gdal_i gdal
	gdal1.8.0
	gdal1.7.0
	gdal1.6.2 gdal1.6.1 gdal1.6.0
	gdal1.5.4 gdal1.5.3 gdal1.5.2 gdal1.5.1 gdal1.5.0
	gdal1.4.5 gdal1.4.4 gdal1.4.3 gdal1.4.2 gdal1.4.1 gdal1.4.0
	gdal1.3.2
	GDAL)

#
# Try only searching directories of CMake variable 'GDAL_DIR' and environment
# variable 'GDAL_DIR'.
#

set(GDAL_DIR_SEARCH ${GDAL_DIR})
IF(GDAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GDAL_DIR_SEARCH}" GDAL_DIR_SEARCH)
    SET(GDAL_DIR_SEARCH ${GDAL_DIR_SEARCH})
    
    # gdal1.3.2 on Ubuntu 7.04 in 2007-2008 (GPlates changeset 2107).
    # gdal1.4.0 on Ubuntu 7.10 in 2008 (GPlates changeset 2222).
    # gdal1.5.0 on Debian testing in 2008 (GPlates changeset 2773).
    # gdal1.5.2 on OpenSUSE 11 in 2008 (GPlates changeset 3954).
    FIND_LIBRARY(GDAL_LIBRARY
      NAMES ${GDAL_LIBRARY_NAMES}
      PATHS ${GDAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "GDAL library"
    )
ENDIF(GDAL_DIR_SEARCH)

set(GDAL_DIR_SEARCH $ENV{GDAL_DIR})
IF(GDAL_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GDAL_DIR_SEARCH}" GDAL_DIR_SEARCH)
    SET(GDAL_DIR_SEARCH ${GDAL_DIR_SEARCH})
    
    FIND_LIBRARY(GDAL_LIBRARY
      NAMES ${GDAL_LIBRARY_NAMES}
      PATHS ${GDAL_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES lib64 lib
      DOC "GDAL library"
    )
ENDIF(GDAL_DIR_SEARCH)

FIND_LIBRARY(GDAL_LIBRARY 
  NAMES ${GDAL_LIBRARY_NAMES}
  PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
    NO_DEFAULT_PATH
    PATH_SUFFIXES lib64 lib
    DOC "GDAL library"
)

#
# Try searching default paths (plus the extras specified in 'PATHS' (which
# looks like it catches alot of defaults anyway).
#

FIND_LIBRARY(GDAL_LIBRARY 
  NAMES ${GDAL_LIBRARY_NAMES}
  PATHS
    "${GDAL_INCLUDE_DIR}/.." # this only really needed on windows platforms
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
    /usr/freeware
    [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;GDAL_ROOT]/lib
  PATH_SUFFIXES lib64 lib
  DOC "GDAL library"
)


include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDAL REQUIRED_VARS GDAL_LIBRARY GDAL_INCLUDE_DIR)

if (GDAL_FOUND AND NOT TARGET GDAL::GDAL)
	add_library(GDAL::GDAL IMPORTED INTERFACE)
	set_target_properties(GDAL::GDAL PROPERTIES INTERFACE_LINK_LIBRARIES "${GDAL_LIBRARY}")
	set_target_properties(GDAL::GDAL PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
		# Include directories (must be specified as a list "a;b;c" to 'set_target_properties')...
		"${GDAL_INCLUDE_DIR};${GDAL_OGR_INCLUDE_DIR};${GDAL_PORT_INCLUDE_DIR};${GDAL_FRMTS_INCLUDE_DIR}")
endif ()

mark_as_advanced(GDAL_LIBRARY GDAL_INCLUDE_DIR)
