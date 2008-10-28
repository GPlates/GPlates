# Finds the International Components for Unicode (ICU) Library
# This module defines a number of key variables and macros. First is 
# ICU_USE_FILE which is the path to a CMake file that can be included to compile
# ICU applications and libraries.  By default, the ICU_CORE libraries are loaded.
# This behavior can be changed by setting one or more 
# of the following variables to true:
#                    ICU_DONT_USE_ICU_CORE
#                    ICU_USE_I18N
#                    ICU_USE_IO
#
#  ICU_FOUND          - True if ICU found.
#  ICU_INCLUDE_DIR   - Directory to include to get ICU headers
#                       Note: always include ICU headers as, e.g., 
#                       unicode/utypes.h
# All the libraries required are stored in a variable called ICU_LIBRARIES.  
# Add this variable to your TARGET_LINK_LIBRARIES.
#

# ----------------------------------------------------------------------------
# If you have installed ICU in a non-standard location (eg, on windows) then you have two
# options. In the following comments, it is assumed that <Your Path>
# points to the root directory of the include directory of ICU. e.g
# If you have put ICU in "C:\Libraries\icu" then <Your Path> is
# "C:\Libraries\icu" and in this directory there will be two
# directories called "include" and "lib".
# 1) Add to the environment variable called ${INCLUDE} to point to the ICU include directory.
# 2) After CMake runs, set ICU_INCLUDE_DIR to <Your Path>/include
# 3) Use CMAKE_INCLUDE_PATH to set a path to <Your Path>. This will allow FIND_PATH()
#    to locate ICU_INCLUDE_DIR by utilizing the PATH_SUFFIXES option. e.g.
#    SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "<Your Path>")

SET(ICU_USE_FILE ${CMAKE_MODULE_PATH}/UseICU.cmake)

#
# Try searching only CMake variable 'ICU_DIR' and environment variable 'ICU_DIR'.
#

SET(ICU_DIR_SEARCH "${ICU_DIR}")
IF(ICU_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${ICU_DIR_SEARCH}" ICU_DIR_SEARCH)
    SET(ICU_DIR_SEARCH ${ICU_DIR_SEARCH})
    
    FIND_PATH(ICU_INCLUDE_DIR unicode/utypes.h
      PATHS ${ICU_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the ICU library")
ENDIF(ICU_DIR_SEARCH)

SET(ICU_DIR_SEARCH "$ENV{ICU_DIR}")
IF(ICU_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${ICU_DIR_SEARCH}" ICU_DIR_SEARCH)
    SET(ICU_DIR_SEARCH ${ICU_DIR_SEARCH})
    
    FIND_PATH(ICU_INCLUDE_DIR unicode/utypes.h
      PATHS ${ICU_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the ICU library")
ENDIF(ICU_DIR_SEARCH)

#
# Try searching only environment variable 'INCLUDE'.
#

SET(ICU_DIR_SEARCH "$ENV{INCLUDE}")
IF(ICU_DIR_SEARCH)
    FILE(TO_CMAKE_PATH "${ICU_DIR_SEARCH}" ICU_DIR_SEARCH)
    SET(ICU_DIR_SEARCH ${ICU_DIR_SEARCH})
    
    FIND_PATH(ICU_INCLUDE_DIR unicode/utypes.h
      PATHS ${ICU_DIR_SEARCH}
      NO_DEFAULT_PATH
      PATH_SUFFIXES include
      DOC "Include directory for the ICU library")
ENDIF(ICU_DIR_SEARCH)


#
# Try searching default paths (plus the extras specified in 'PATHS' (which looks like it catches alot of defaults anyway).
#

find_path(
  ICU_INCLUDE_DIR
  NAMES unicode/utypes.h
  PATHS
  ~/Library/Frameworks/icu.framework/Headers
  /Library/Frameworks/icu.framework/Headers
  /usr/local/include/icu
  /usr/local/include/ICU
  /usr/local/include
  /usr/include/icu
  /usr/include/ICU
  /usr/include
  /sw/include/icu 
  /sw/include/ICU 
  /sw/include # Fink
  /opt/local/include/icu
  /opt/local/include/ICU
  /opt/local/include # DarwinPorts
  /opt/csw/include/icu
  /opt/csw/include/ICU
  /opt/csw/include # Blastwave
  /opt/include/icu
  /opt/include/ICU
  /opt/include
  DOC "Include directory for the ICU library")
#mark_as_advanced(ICU_INCLUDE_DIR)

MACRO (FIND_ICU_LIBRARY _ICU_LIBRARY _ICU_FOUND)
    SET(ICU_DIR_SEARCH "${ICU_DIR}")
    IF(ICU_DIR_SEARCH)
        FILE(TO_CMAKE_PATH "${ICU_DIR_SEARCH}" ICU_DIR_SEARCH)
        SET(ICU_DIR_SEARCH ${ICU_DIR_SEARCH})
        
        # Look for the library using the 'ICU_DIR' environment variable.
        find_library(
          ${_ICU_LIBRARY}
          NAMES ${ARGN}
          PATHS ${ICU_DIR_SEARCH}
          NO_DEFAULT_PATH
          PATH_SUFFIXES lib64 lib
          DOC "ICU library")
    ENDIF(ICU_DIR_SEARCH)

    SET(ICU_DIR_SEARCH "$ENV{ICU_DIR}")
    IF(ICU_DIR_SEARCH)
        FILE(TO_CMAKE_PATH "${ICU_DIR_SEARCH}" ICU_DIR_SEARCH)
        SET(ICU_DIR_SEARCH ${ICU_DIR_SEARCH})
        
        # Look for the library using the 'ICU_DIR' environment variable.
        find_library(
          ${_ICU_LIBRARY}
          NAMES ${ARGN}
          PATHS ${ICU_DIR_SEARCH}
          NO_DEFAULT_PATH
          PATH_SUFFIXES lib64 lib
          DOC "ICU library")
    ENDIF(ICU_DIR_SEARCH)

    # Add a search directory that is relative to the include directory.
    set(ICU_DIR_SEARCH "${ICU_INCLUDE_DIR}/.."
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
      ${_ICU_LIBRARY}
      NAMES ${ARGN}
      PATH_SUFFIXES lib64 lib
      PATHS ${ICU_DIR_SEARCH}
      DOC "ICU library")

    #mark_as_advanced(${_ICU_LIBRARY})

    if(${_ICU_LIBRARY})
      set(${_ICU_FOUND} 1)
    endif(${_ICU_LIBRARY})
ENDMACRO (FIND_ICU_LIBRARY _ICU_LIBRARY _ICU_FOUND)

if (ICU_INCLUDE_DIR)
    FIND_ICU_LIBRARY(ICU_CORE_LIBRARY ICU_CORE_FOUND icuuc cygicuuc cygicuuc32)
    FIND_ICU_LIBRARY(ICU_I18N_LIBRARY ICU_I18N_FOUND icuin icui18n cygicuin cygicuin32)
    FIND_ICU_LIBRARY(ICU_IO_LIBRARY ICU_IO_FOUND icuio cygicuio cygicuio32)
    FIND_ICU_LIBRARY(ICU_DATA_LIBRARY ICU_DATA_FOUND icudata cygicudata cygicudata32 icudt cygicudt cygicudt32)
    FIND_ICU_LIBRARY(ICU_LE_LIBRARY ICU_LE_FOUND icule cygicule cygicule32)
    FIND_ICU_LIBRARY(ICU_LX_LIBRARY ICU_LX_FOUND iculx cygiculx cygiculx32)
    FIND_ICU_LIBRARY(ICU_TU_LIBRARY ICU_TU_FOUND icutu cygicutu cygicutu32)
endif (ICU_INCLUDE_DIR)
