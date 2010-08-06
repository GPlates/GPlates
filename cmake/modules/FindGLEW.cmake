# Finds the OpenGL Extension Wrangler Library (GLEW).
# This module defines a number of key variables and macros.
#
#  GLEW_FOUND         - True if GLEW found.
#  GLEW_INCLUDE_DIR   - Directory to include to get GLEW headers
#                       Note: always include GLEW headers as, e.g., 
#                       GL/glew.h
#  GLEW_LIBRARY       - The GLEW library (note: on windows you'll need to add the GLEW directory
#                       containing "glew32.dll" to the PATH environment variable).
#                       Add this variable to your TARGET_LINK_LIBRARIES.
#

# ----------------------------------------------------------------------------
# If you have installed GLEW in a non-standard location (eg, on windows) then you have two
# options. In the following comments, it is assumed that <Your Path>
# points to the root directory of the include directory of GLEW. e.g
# If you have put GLEW in "C:\Libraries\glew" then <Your Path> is
# "C:\Libraries\glew" and in this directory there will be two
# directories called "include" and "lib".
# 1) Add to the environment variable called ${INCLUDE} to point to the GLEW include directory.
# 2) After CMake runs, set GLEW_INCLUDE_DIR to <Your Path>/include
# 3) Use CMAKE_INCLUDE_PATH to set a path to <Your Path>. This will allow FIND_PATH()
#    to locate GLEW_INCLUDE_DIR by utilizing the PATH_SUFFIXES option. e.g.
#    SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "<Your Path>")

#
# Try searching only CMake variable 'GLEW_DIR' and environment variable 'GLEW_DIR'.
#

SET(GLEW_DIR_SEARCH "${GLEW_DIR}")
IF(GLEW_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GLEW_DIR_SEARCH}" GLEW_DIR_SEARCH)
    SET(GLEW_DIR_SEARCH ${GLEW_DIR_SEARCH})
    
    FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h
      PATHS ${GLEW_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GLEW library")
ENDIF(GLEW_DIR_SEARCH)

SET(GLEW_DIR_SEARCH "$ENV{GLEW_DIR}")
IF(GLEW_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GLEW_DIR_SEARCH}" GLEW_DIR_SEARCH)
    SET(GLEW_DIR_SEARCH ${GLEW_DIR_SEARCH})
    
    FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h
      PATHS ${GLEW_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GLEW library")
ENDIF(GLEW_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(GLEW_DIR_SEARCH "$ENV{INCLUDE}")
IF(GLEW_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${GLEW_DIR_SEARCH}" GLEW_DIR_SEARCH)
    SET(GLEW_DIR_SEARCH ${GLEW_DIR_SEARCH})
    
    FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h
      PATHS ${GLEW_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the GLEW library")
ENDIF(GLEW_DIR_SEARCH)


#
# Try searching default paths (plus the extras specified in 'PATHS' (which looks like it catches alot of defaults anyway).
#

find_path(
  GLEW_INCLUDE_DIR
  NAMES GL/glew.h
  PATHS
  ~/Library/Frameworks/glew.framework/Headers
  /Library/Frameworks/glew.framework/Headers
  /usr/local/include/glew
  /usr/local/include/GLEW
  /usr/local/include
  /usr/include/glew
  /usr/include/GLEW
  /usr/include
  /sw/include/glew 
  /sw/include/GLEW 
  /sw/include # Fink
  /opt/local/include/glew
  /opt/local/include/GLEW
  /opt/local/include # DarwinPorts
  /opt/csw/include/glew
  /opt/csw/include/GLEW
  /opt/csw/include # Blastwave
  /opt/include/glew
  /opt/include/GLEW
  /opt/include
  DOC "Include directory for the GLEW library")
#mark_as_advanced(GLEW_INCLUDE_DIR)

MACRO (FIND_GLEW_LIBRARY _GLEW_LIBRARY _GLEW_FOUND)
    SET(GLEW_DIR_SEARCH "${GLEW_DIR}")
    IF(GLEW_DIR_SEARCH)
        FILE(TO_CMAKE_PATH "${GLEW_DIR_SEARCH}" GLEW_DIR_SEARCH)
        SET(GLEW_DIR_SEARCH ${GLEW_DIR_SEARCH})
        
        # Look for the library using the 'GLEW_DIR' environment variable.
        find_library(
          ${_GLEW_LIBRARY}
          NAMES ${ARGN}
          PATHS ${GLEW_DIR_SEARCH}
          NO_DEFAULT_PATH
          PATH_SUFFIXES lib64 lib
          DOC "GLEW library")
    ENDIF(GLEW_DIR_SEARCH)

    SET(GLEW_DIR_SEARCH "$ENV{GLEW_DIR}")
    IF(GLEW_DIR_SEARCH)
        FILE(TO_CMAKE_PATH "${GLEW_DIR_SEARCH}" GLEW_DIR_SEARCH)
        SET(GLEW_DIR_SEARCH ${GLEW_DIR_SEARCH})
        
        # Look for the library using the 'GLEW_DIR' environment variable.
        find_library(
          ${_GLEW_LIBRARY}
          NAMES ${ARGN}
          PATHS ${GLEW_DIR_SEARCH}
          NO_DEFAULT_PATH
          PATH_SUFFIXES lib64 lib
          DOC "GLEW library")
    ENDIF(GLEW_DIR_SEARCH)

    # Add a search directory that is relative to the include directory.
    set(GLEW_DIR_SEARCH "${GLEW_INCLUDE_DIR}/.."
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt
        /usr/freeware)

    # Look for the library in the standard paths (this should work on Unix systems).
    find_library(
      ${_GLEW_LIBRARY}
      NAMES ${ARGN}
      PATH_SUFFIXES lib64 lib
      PATHS ${GLEW_DIR_SEARCH}
      DOC "GLEW library")

    #mark_as_advanced(${_GLEW_LIBRARY})

    if(${_GLEW_LIBRARY})
      set(${_GLEW_FOUND} 1)
    endif(${_GLEW_LIBRARY})
ENDMACRO (FIND_GLEW_LIBRARY _GLEW_LIBRARY _GLEW_FOUND)

if (GLEW_INCLUDE_DIR)
    FIND_GLEW_LIBRARY(GLEW_LIBRARY GLEW_CORE_FOUND GLEW glew GLEW32 glew32)
endif (GLEW_INCLUDE_DIR)

# Found GLEW only if both include directory and library found.
SET(GLEW_FOUND "NO")
IF(GLEW_CORE_FOUND AND GLEW_INCLUDE_DIR)
  SET(GLEW_FOUND "YES")
ENDIF(GLEW_CORE_FOUND AND GLEW_INCLUDE_DIR)
