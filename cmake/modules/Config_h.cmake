#
# Generate 'global/config.h' file from 'global/config.h.in'.
#

# For now only setting the variables currently used by GPlates:
#   HAVE_GDAL_OGRSF_FRMTS_H (set in 'FindGDAL.cmake').
set(PACKAGE_IS_BETA 1)

# Tell GPlates that we have a valid 'global/config.h' file.
add_definitions(-DHAVE_CONFIG_H)

# Convert 'global/config.h.in' file to 'global/config.h'.
set(CONFIG_H_IN_FILE "${GPlates_SOURCE_DIR}/src/global/config.h.in")
set(CONFIG_H_OUT_FILE "${GPlates_BINARY_DIR}/src/global/config.h")
configure_file(${CONFIG_H_IN_FILE} ${CONFIG_H_OUT_FILE})

# Include directory where generated 'global/config.h' will go.
include_directories(${GPlates_BINARY_DIR}/src)
