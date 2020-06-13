# Create a "make doc-cpp" target using Doxygen
# Prototype:
#     GENERATE_DOCUMENTATION(doxygen_config_file)
# Parameters:
# doxygen_config_file Doxygen configuration file.


MACRO(GENERATE_DOCUMENTATION DOXYGEN_CONFIG_FILE_IN DOXYGEN_CONFIG_FILE)
    FIND_PACKAGE(Doxygen)
    SET(DOXYFILE_FOUND false)
    IF(EXISTS ${DOXYGEN_CONFIG_FILE_IN})
        SET(DOXYFILE_FOUND true)
    ENDIF(EXISTS ${DOXYGEN_CONFIG_FILE_IN})

    IF( DOXYGEN_FOUND )
        IF( DOXYFILE_FOUND )
            # Files to clean.
            SET(CLEAN_FILES )
            
            # Insert variables into the doxygen config file.
            SET(PACKAGE_NAME \"${GPLATES_PACKAGE_NAME}\")
            SET(PACKAGE_VERSION \"${GPlates_VERSION}\")
            SET(DOXYGEN_INPUT ${GPLATES_DOXYGEN_INPUT})
            SET(HAVE_DOT NO)
            SET(DOT_PATH )
            IF (DOXYGEN_DOT_FOUND)
                SET(HAVE_DOT YES)
                # Add the doxygen dot path so doxygen can find it.
                IF (DOXYGEN_DOT_PATH)
                    SET(DOT_PATH \"${DOXYGEN_DOT_PATH}\")
                ENDIF (DOXYGEN_DOT_PATH)
            ENDIF (DOXYGEN_DOT_FOUND)
            CONFIGURE_FILE(${DOXYGEN_CONFIG_FILE_IN} ${DOXYGEN_CONFIG_FILE} @ONLY)
            
            # Add custom command.
            # ADD_CUSTOM_COMMAND(
                # OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html
                #COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/html
                # COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYGEN_CONFIG_FILE}"
                # DEPENDS ${DOXYGEN_CONFIG_FILE}
                # WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

            # Add target
            ADD_CUSTOM_TARGET(doc-cpp
                # Best to remove 'html' subdirectory before running doxygen as I've heard it can cause problems...
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/html
                COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYGEN_CONFIG_FILE}"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

            ##Add .tag file and generated documentation to the list of files we must erase when cleaning 

            ##Read doxygen configuration file
            # FILE( READ ${DOXYGEN_CONFIG_FILE} DOXYFILE_CONTENTS )
            # STRING( REGEX REPLACE "\n" ";" DOXYFILE_LINES ${DOXYFILE_CONTENTS} )

            ##Parse .tag filename and add to list of files to delete if it exists
            # FOREACH( DOXYLINE ${DOXYFILE_CONTENTS} )
                # STRING( REGEX REPLACE ".*GENERATE_TAGFILE *= *([^ ^\n]+).*" "\\1" DOXYGEN_TAG_FILE ${DOXYLINE} ) 
            # ENDFOREACH( DOXYLINE )
            # SET(CLEAN_FILES "${CLEAN_FILES} ${DOXYGEN_TAG_FILE}")

            ##Parse doxygen output doc dir and add to list of files to delete if it exists 
            # FOREACH( DOXYLINE ${DOXYFILE_CONTENTS} )
                # STRING( REGEX REPLACE ".*OUTPUT_DIRECTORY *= *([^ ^\n]+).*" "\\1" DOXYGEN_DOC_DIR ${DOXYLINE} ) 
            # ENDFOREACH( DOXYLINE )
            # SET(CLEAN_FILES "${CLEAN_FILES} ${DOXYGEN_DOC_DIR} ${DOXYGEN_DOC_DIR}.dir")
            
            # SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${CLEAN_FILES}")

        ELSE( DOXYFILE_FOUND )
            MESSAGE( STATUS "Doxygen configuration file not found - Documentation will not be generated" ) 
        ENDIF( DOXYFILE_FOUND )
    ELSE(DOXYGEN_FOUND)
        MESSAGE(STATUS "Doxygen not found - Documentation will not be generated")
    ENDIF(DOXYGEN_FOUND)
ENDMACRO(GENERATE_DOCUMENTATION)
