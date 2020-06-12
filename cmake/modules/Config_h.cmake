#
# Set some variables needed when generating 'global/config.h' file from 'global/config.h.in'.
#

# For now only setting the variables currently used by GPlates:

IF(GDAL_INCLUDE_DIR)
  # Set HAVE_GDAL_OGRSF_FRMTS_H to 1 if the "ogrsf_frmts.h" file
  # lives in a "gdal" directory.
  # This is used in GPlates to determine which file to '#include'.
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/ogrsf_frmts.h")
    set(HAVE_GDAL_OGRSF_FRMTS_H 1)
  ENDIF()
  
  # Set HAVE_GDAL_VERSION_H to 1 if the "gdal_version.h" file exists.
  # If so then also set HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX
  # or HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX to 1 if we need
  # "#include <gdal/gdal_version.h>" in our C++ code instead of "#include <gdal_version.h>".
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
  ENDIF()
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
	set(HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX 1)
  ENDIF()
  IF (EXISTS "${GDAL_INCLUDE_DIR}/GDAL/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
	set(HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX 1)
  ENDIF()
ENDIF()

FOREACH(_PROJ_INCLUDE_DIR ${PROJ4_INCLUDE_DIRS})
  # If have the Proj6 header ("proj.h").
  IF (EXISTS "${_PROJ_INCLUDE_DIR}/proj.h")
    set(HAVE_PROJ_H 1)
  ENDIF()
  
  # If have the Proj4 header ("proj_api.h").
  IF (EXISTS "${_PROJ_INCLUDE_DIR}/proj_api.h")
    set(HAVE_PROJ_API_H 1)
  ENDIF()
ENDFOREACH()

# The following system header files are found in the "system-fixes" directory.
# If they also exist in the expected places on the user's system, we set a
# variable to indicate the path to the installed version of the header file.
SET (BOOST_CSTDINT_HPP_PATH "${Boost_INCLUDE_DIR}/boost/cstdint.hpp")
IF (NOT EXISTS ${BOOST_CSTDINT_HPP_PATH})
	SET (BOOST_CSTDINT_HPP_PATH "")
ENDIF()
