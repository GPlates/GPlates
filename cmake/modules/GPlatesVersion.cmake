#
# Generate 'global/Constants.cc' file from 'global/Constants.cc.in'.
#

# Currently only uses the GPLATES_VERSION_STRING which is already defined in "ConfigDefault.cmake".

set(CONSTANTS_CC_IN_FILE "${GPlates_SOURCE_DIR}/src/global/Constants.cc.in")
set(CONSTANTS_CC_OUT_FILE "${GPlates_SOURCE_DIR}/src/global/Constants.cc")

configure_file(${CONSTANTS_CC_IN_FILE} ${CONSTANTS_CC_OUT_FILE})
