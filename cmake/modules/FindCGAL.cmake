#
# The following module is based on FindVTK.cmake
#

# - Find a CGAL installation or binary tree.
# The following variables are set if CGAL is found.  If CGAL is not
# found, CGAL_FOUND is set to false.
#
#  CGAL_FOUND         - Set to true when CGAL is found.
#  CGAL_USE_FILE      - CMake file to use CGAL.
#

# Construct consitent error messages for use below.
set(CGAL_DIR_DESCRIPTION "directory containing CGALConfig.cmake. This is either the binary directory where CGAL was configured or PREFIX/lib/CGAL for an installation.")
set(CGAL_DIR_MESSAGE     "CGAL not found.  Set the CGAL_DIR cmake variable or environment variable to the ${CGAL_DIR_DESCRIPTION}")
 
set(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)
 
if ( NOT CGAL_DIR )
  
  # Get the system search path as a list.
  if(UNIX)
    string(REGEX MATCHALL "[^:]+" CGAL_DIR_SEARCH1 "$ENV{PATH}")
  else(UNIX)
    string(REGEX REPLACE "\\\\" "/" CGAL_DIR_SEARCH1 "$ENV{PATH}")
  endif(UNIX)
  
  string(REGEX REPLACE "/;" ";" CGAL_DIR_SEARCH2 "${CGAL_DIR_SEARCH1}")

  # Construct a set of paths relative to the system search path.
  set(CGAL_DIR_SEARCH "")
  
  foreach(dir ${CGAL_DIR_SEARCH2})
  
    set(CGAL_DIR_SEARCH ${CGAL_DIR_SEARCH} ${dir}/../lib/CGAL )
      
  endforeach(dir)


  #
  # Look for an installation or build tree.
  #
  find_path(CGAL_DIR CGALConfig.cmake

    # Look for an environment variable CGAL_DIR.
    $ENV{CGAL_DIR}

    # Look in places relative to the system executable search path.
    ${CGAL_DIR_SEARCH}

    # Look in standard UNIX install locations.
    /usr/local/lib/CGAL
    /usr/lib/CGAL
    /opt/local/lib/CGAL

    # Read from the CMakeSetup registry entries.  It is likely that
    # CGAL will have been recently built.
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild1]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild2]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild3]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild4]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild5]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild6]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild7]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild8]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild9]
    [HKEY_CURRENT_USER\\Software\\Kitware\\CMakeSetup\\Settings\\StartPath;WhereBuild10]

    # Help the user find it if we cannot.
    DOC "The ${CGAL_DIR_DESCRIPTION}"
  )
  
endif ( NOT CGAL_DIR )

# On Ubuntu 9.10, the CMake script adds -g because the CGAL CMake script adds the -g
# flag even though it is a release build.

#if ( CGAL_DIR )
# 
# if ( EXISTS "${CGAL_DIR}/CGALConfig.cmake" )
#   include( "${CGAL_DIR}/CGALConfig.cmake" )
#   set( CGAL_FOUND TRUE )
# endif ( EXISTS "${CGAL_DIR}/CGALConfig.cmake" )
#
#else ( CGAL_DIR )

  # Versions of CGAL before 3.4 don't use cmake so we need to search for the
  # include/lib directories ourself and we also need to add the appropriate
  # compiler options (CGAL requires us to use the same options as were used
  # to build the CGAL library).
  # This is currently necessary only for Linux systems (eg, Ubuntu 8.04 has
  # CGAL 3.3.1 as a package) - using the package manager avoids requiring
  # the user to download and install CGAL from source code.
  if (NOT WIN32 AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
    # Search for the CGAL include directory and library.
    # We are only interested in libcgal-dev packages on linux which should be
    # installed in standard locations and the library should be called "libCGAL.so".
    find_path(CGAL_INCLUDE_DIR CGAL/version.h DOC "Include directory for the CGAL library")
    find_library(CGAL_LIBRARY NAMES CGAL DOC "CGAL library")

    if (CGAL_INCLUDE_DIR AND CGAL_LIBRARY)
      set(CGAL_FOUND TRUE)
      set(CGAL_USE_FILE "${CMAKE_MODULE_PATH}/UseCGAL_3_3.cmake")
    endif (CGAL_INCLUDE_DIR AND CGAL_LIBRARY)

    if (NOT CGAL_FOUND)
      # This is a linux only error message.
      set(CGAL_DIR_MESSAGE "CGAL not found.  Either install the 'libcgal-dev' package (if using CGAL 3.3 or below) or set the CGAL_DIR cmake variable or environment variable (if using CGAL 3.4 or above) to the ${CGAL_DIR_DESCRIPTION}")
    endif (NOT CGAL_FOUND)
  endif (NOT WIN32 AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)

#endif ( CGAL_DIR )

if( NOT CGAL_FOUND)
  if(CGAL_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR ${CGAL_DIR_MESSAGE})
  else(CGAL_FIND_REQUIRED)
    if(NOT CGAL_FIND_QUIETLY)
      MESSAGE(STATUS ${CGAL_DIR_MESSAGE})
    endif(NOT CGAL_FIND_QUIETLY)
  endif(CGAL_FIND_REQUIRED)
endif( NOT CGAL_FOUND)
