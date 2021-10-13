#
# Generate 'global/config.h' file from 'global/config.h.in'.
#

# For now only setting the variables currently used by GPlates:

set(PACKAGE_IS_BETA 1)

IF(GDAL_INCLUDE_DIR)
  # Set HAVE_GDAL_OGRSF_FRMTS_H to 1 if the "ogrsf_frmts.h" file
  # lives in a "gdal" directory.
  # This is used in GPlates to determine which file to '#include'.
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/ogrsf_frmts.h")
    set(HAVE_GDAL_OGRSF_FRMTS_H 1)
  ENDIF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/ogrsf_frmts.h")
  
  # Set HAVE_GDAL_VERSION_H to 1 if the "gdal_version.h" file exists.
  # If so then also set HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX
  # or HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX to 1 if we need
  # "#include <gdal/gdal_version.h>" in our C++ code instead of "#include <gdal_version.h>".
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
  ENDIF (EXISTS "${GDAL_INCLUDE_DIR}/gdal_version.h")
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
	set(HAVE_GDAL_VERSION_H_LOWERCASE_GDAL_PREFIX 1)
  ENDIF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/gdal_version.h")
  IF (EXISTS "${GDAL_INCLUDE_DIR}/GDAL/gdal_version.h")
    set(HAVE_GDAL_VERSION_H 1)
	set(HAVE_GDAL_VERSION_H_UPPERCASE_GDAL_PREFIX 1)
  ENDIF (EXISTS "${GDAL_INCLUDE_DIR}/GDAL/gdal_version.h")
ENDIF(GDAL_INCLUDE_DIR)

# The following system header files are found in the "system-fixes" directory.
# If they also exist in the expected places on the user's system, we set a
# variable to indicate the path to the installed version of the header file.
SET (BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH "${Boost_INCLUDE_DIR}/boost/math/special_functions/fpclassify.hpp")
IF (NOT EXISTS ${BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH})
	SET (BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH "")
ENDIF (NOT EXISTS ${BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH})

SET (BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH "${Boost_INCLUDE_DIR}/boost/math/tools/real_cast.hpp")
IF (NOT EXISTS ${BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH})
	SET (BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH "")
ENDIF (NOT EXISTS ${BOOST_MATH_TOOLS_REAL_CAST_HPP_PATH})

SET (BOOST_CSTDINT_HPP_PATH "${Boost_INCLUDE_DIR}/boost/cstdint.hpp")
IF (NOT EXISTS ${BOOST_CSTDINT_HPP_PATH})
	SET (BOOST_CSTDINT_HPP_PATH "")
ENDIF (NOT EXISTS ${BOOST_CSTDINT_HPP_PATH})

# controls to find patched Constrained_Delaunay_triangulation_2.h 
SET (GCAL_PATCH_CDT2_H_PATH "${CGAL_INCLUDE_DIR}/cgal/Constrained_Delaunay_triangulation_2.h") 
IF (EXISTS ${GCAL_PATCH_CDT2_H_PATH}) 
    # uncomment for CGAL 3.7
    # SET (HAVE_GCAL_PATCH_CDT2_H 1) 
ENDIF (EXISTS ${GCAL_PATCH_CDT2_H_PATH})

# Tell GPlates that we have a valid 'global/config.h' file.
add_definitions(-DHAVE_CONFIG_H)

# Convert 'global/config.h.in' file to 'global/config.h'.
set(CONFIG_H_IN_FILE "${GPlates_SOURCE_DIR}/src/global/config.h.in")
set(CONFIG_H_OUT_FILE "${CMAKE_BINARY_DIR}/src/global/config.h")
configure_file(${CONFIG_H_IN_FILE} ${CONFIG_H_OUT_FILE})
