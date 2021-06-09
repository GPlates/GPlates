#
# Set some variables needed when generating 'global/config.h' file from 'global/config.h.in'.
#

# For now only setting the variables currently used by GPlates:

# Determine which PROJ header to include.
#
# For Proj5+ we should include 'proj.h' (the modern API).
# For Proj4 we can only include 'proj_api.h' (the old API).
#
# Note that Proj8 removed the Proj4 header ('proj_api.h') but both headers
# exist in Proj versions 5, 6 and 7 (where we choose 'proj.h').
FOREACH(_PROJ_INCLUDE_DIR ${PROJ_INCLUDE_DIRS})
  # If have the Proj5+ header ("proj.h").
  IF (EXISTS "${_PROJ_INCLUDE_DIR}/proj.h")
    set(GPLATES_HAVE_PROJ_H 1)
  ENDIF()
  
  # If have the Proj4 header ("proj_api.h").
  IF (EXISTS "${_PROJ_INCLUDE_DIR}/proj_api.h")
    set(GPLATES_HAVE_PROJ_API_H 1)
  ENDIF()
ENDFOREACH()

# The following system header files are found in the "system-fixes" directory.
# If they also exist in the expected places on the user's system, we set a
# variable to indicate the path to the installed version of the header file.
# The 'src/system-fixes/boost/cstdint.hpp' gets included by our source code and
# it, in turn, uses this variable to include the system <boost/cstdint.hpp>.
SET (BOOST_CSTDINT_HPP_PATH "${Boost_INCLUDE_DIR}/boost/cstdint.hpp")
IF (NOT EXISTS "${BOOST_CSTDINT_HPP_PATH}")
	SET (BOOST_CSTDINT_HPP_PATH "")
ENDIF()

# Do we have boost.python.numpy?
#
# Only available for Boost >= 1.63, and if boost.python.numpy installed since
# it's currently optional (because we don't actually use it yet).
if (TARGET Boost::${GPLATES_BOOST_PYTHON_NUMPY_COMPONENT_NAME})
  set(GPLATES_HAVE_BOOST_PYTHON_NUMPY 1)
endif()

# Do we have the NumPy C-API include directories?
#
# If GPLATES_Python_NumPy_INCLUDE_DIRS is set then it's also been added
# to the target include directories so we don't have to do that here
# (in other words in the source code we just need "#include <numpy/arrayobject.h>").
if (GPLATES_Python_NumPy_INCLUDE_DIRS)
  # Also make sure "numpy/arrayobject.h" actually exists.
  if (EXISTS "${GPLATES_Python_NumPy_INCLUDE_DIRS}/numpy/arrayobject.h")
    set(GPLATES_HAVE_NUMPY_C_API 1)
  endif()
endif()
