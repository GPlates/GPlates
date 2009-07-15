#
# Generate 'global/config.h' file from 'global/config.h.in'.
#

# For now only setting the variables currently used by GPlates:

set(PACKAGE_IS_BETA 1)

# Set HAVE_GDAL_OGRSF_FRMTS_H to 1 if the "ogrsf_frmts.h" file
# lives in a "gdal" directory.
# This is used in GPlates to determine which file to '#include'.
IF(GDAL_INCLUDE_DIR)
  IF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/ogrsf_frmts.h")
    set(HAVE_GDAL_OGRSF_FRMTS_H 1)
  ENDIF (EXISTS "${GDAL_INCLUDE_DIR}/gdal/ogrsf_frmts.h")
ENDIF(GDAL_INCLUDE_DIR)

# Tell GPlates that we have a valid 'global/config.h' file.
add_definitions(-DHAVE_CONFIG_H)

# Convert 'global/config.h.in' file to 'global/config.h'.
set(CONFIG_H_IN_FILE "${GPlates_SOURCE_DIR}/src/global/config.h.in")
set(CONFIG_H_OUT_FILE "${GPlates_SOURCE_DIR}/src/global/config.h")
configure_file(${CONFIG_H_IN_FILE} ${CONFIG_H_OUT_FILE})
