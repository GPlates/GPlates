# FindPROJ
# --------
# 
# Find the native PROJ includes and library.
# 
# Input Variables
# ^^^^^^^^^^^^^^^
# 
# The following variables may be set to influence this module's behavior:
# 
# ``PROJ_VERBOSE``
#   to output a log of this module.
# 
# Imported Targets
# ^^^^^^^^^^^^^^^^
# 
# This module defines :prop_tgt:`IMPORTED` target ``PROJ::PROJ``, if PROJ has been found.
# 
# Result Variables
# ^^^^^^^^^^^^^^^^
# 
# This module defines the following variables:
# 
# ::
# 
#   PROJ_INCLUDE_DIRS   - where to find proj.h, etc.
#   PROJ_LIBRARIES      - List of libraries when using PROJ.
#   PROJ_FOUND          - True if PROJ found.

include(FindPackageHandleStandardArgs)

# First look for the CONFIG package (provided with the installed Proj library).
#
# NOTE: Starting with Proj7 there is also the PROJ name (not just PROJ4).
#       And a future Proj release will deprecate the PROJ4 name
#       (with the PROJ4::proj eventually being retired in preference to PROJ::proj).
#       So first we'll try PROJ, then PROJ4 (but note that PROJ4 can mean any proj version, eg, 4,5,6,7).
#       And if we only find PROJ4 we'll still create everything under the PROJ name (so that caller only see PROJ name).
#
# First try PROJ.
find_package(PROJ CONFIG QUIET)
if (PROJ_FOUND)
  find_package_handle_standard_args(PROJ CONFIG_MODE)

  if (PROJ_VERBOSE)
    message(STATUS "FindPROJ: found PROJ CMake config file.")
  endif()

  return()
endif()
#
# Then try PROJ4.
find_package(PROJ4 CONFIG QUIET)
if (PROJ4_FOUND)
  set(PROJ_FOUND TRUE)

  if (PROJ_VERBOSE)
    message(STATUS "FindPROJ: found PROJ4 CMake config file.")
  endif()

  if (TARGET PROJ4::proj)
    # There was a CONFIG package and it defined a PROJ4::proj target.
    # But we need to return a PROJ::proj target, so create an alias.
    add_library(PROJ::proj ALIAS PROJ4::proj)
    if (PROJ_VERBOSE)
      message(STATUS "FindPROJ: created alias PROJ::proj to target PROJ4::proj.")
    endif()
  else()
    # There was a CONFIG package but it defined a 'proj' target instead of a 'PROJ4::proj' target.
    # So make 'PROJ::proj' alias 'proj'.
    # But before we can do this we first need to promote 'proj' to global visibility (requires CMake 3.11 or above).
    set_target_properties(proj PROPERTIES IMPORTED_GLOBAL TRUE)
    # Also it seems that, while the 'proj' target has set the library import location, it doesn't set the location of the include directories.
    set_target_properties(proj PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PROJ4_INCLUDE_DIRS}")
    add_library(PROJ::proj ALIAS proj)
    if (PROJ_VERBOSE)
      message(STATUS "FindPROJ: created alias PROJ::proj to target proj.")
    endif()
  endif()

  # We now have a PROJ::proj target but we should also use the same variables as the *modern* CONFIG package.
  set(PROJ_INCLUDE_DIRS ${PROJ4_INCLUDE_DIRS})
  set(PROJ_LIBRARIES ${PROJ4_LIBRARIES})
  if (PROJ_VERBOSE)
    message(STATUS "FindPROJ: created variable PROJ_INCLUDE_DIRS: ${PROJ_INCLUDE_DIRS}")
    message(STATUS "FindPROJ: created variable PROJ_LIBRARIES: ${PROJ_LIBRARIES}")
  endif()

  return()
endif()

#
# Proj does not have a CONFIG package (at least not all platforms were installed as a CMake package) but more recent versions should.
# Note that Proj 4.9 is still used by Ubuntu Bionic (18.04).
#

if (PROJ_VERBOSE)
  message(STATUS "FindPROJ: did not find PROJ CMake config file. Searching for libraries.")
endif()

# Find the PROJ header location.
find_path(PROJ_INCLUDE_DIR
  NAMES proj.h proj_api.h
  PATHS
    # Macports stores Proj in locations that are difficult for CMake to find without some help
    # (even when the /opt/local/ prefix is used, eg, via CMAKE_PREFIX_PATH, or via the PATH environment variable)...
    /opt/local/lib/proj8/include
    /opt/local/lib/proj7/include
    /opt/local/lib/proj6/include
    /opt/local/lib/proj5/include
    /opt/local/lib/proj49/include)

# Find the PROJ library.
find_library(PROJ_LIBRARY
  NAMES proj proj_i
  NAMES_PER_DIR
  PATHS
    # Macports stores Proj in locations that are difficult for CMake to find without some help
    # (even when the /opt/local/ prefix is used, eg, via CMAKE_PREFIX_PATH, or via the PATH environment variable)...
    /opt/local/lib/proj8/lib
    /opt/local/lib/proj7/lib
    /opt/local/lib/proj6/lib
    /opt/local/lib/proj5/lib
    /opt/local/lib/proj49/lib)

# Make sure we found the PROJ include header location and library.
find_package_handle_standard_args(PROJ REQUIRED_VARS PROJ_LIBRARY PROJ_INCLUDE_DIR)

mark_as_advanced(PROJ_INCLUDE_DIR PROJ_LIBRARY)

# Create the same variables as a PROJ CONFIG package.
set(PROJ_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
set(PROJ_LIBRARIES ${PROJ_LIBRARY})

if (PROJ_VERBOSE)
  message(STATUS "FindPROJ: located PROJ_INCLUDE_DIRS: ${PROJ_INCLUDE_DIRS}")
  message(STATUS "FindPROJ: located PROJ_LIBRARIES: ${PROJ_LIBRARIES}")
endif()

# Create the PROJ::proj target.
add_library(PROJ::proj IMPORTED INTERFACE)
set_target_properties(PROJ::proj PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${PROJ_INCLUDE_DIRS}")
set_target_properties(PROJ::proj PROPERTIES INTERFACE_LINK_LIBRARIES "${PROJ_LIBRARIES}")

if (PROJ_VERBOSE)
  message(STATUS "FindPROJ: created target PROJ::proj")
endif()
