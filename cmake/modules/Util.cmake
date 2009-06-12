# If we are using pre-compiled headers then include the macros required to use them.
# 'GPLATES_USE_PCH' should be added/modified in 'ConfigUser.cmake' only.
if (GPLATES_USE_PCH)
include (PCHSupport)
endif (GPLATES_USE_PCH)

# Parses header files (passed as macro arguments) and returns list of headers containing "Q_OBJECT" .
MACRO (GPLATES_GET_MOC_HEADERS _moc_HDRS )
    set(${_moc_HDRS} )
    FOREACH(hdr ${ARGN})
        GET_FILENAME_COMPONENT(abs_HDR ${hdr} ABSOLUTE)
        IF ( EXISTS ${abs_HDR} )
            FILE(READ ${abs_HDR} file_contents)
            STRING(REGEX MATCH "Q_OBJECT" match "${file_contents}")
            IF(match)
                list(APPEND ${_moc_HDRS} ${hdr})
            ENDIF(match)
        ENDIF ( EXISTS ${abs_HDR} )
    ENDFOREACH(hdr ${ARGN})
ENDMACRO (GPLATES_GET_MOC_HEADERS _moc_HDRS )

# Add header files to the list of source files.
# This is useful for Visual Studio as it lists the header files in the IDE.
MACRO (GPLATES_ADD_HEADERS_TO_SOURCE_FILES _utils_srcs )
    if(MSVC)
        set(${_utils_srcs} ${${_utils_srcs}} ${ARGN})
    endif(MSVC)
ENDMACRO (GPLATES_ADD_HEADERS_TO_SOURCE_FILES)

# For Visual Studio this macro groups the files into folders according to file type.
MACRO (GPLATES_GROUP_SOURCE_FILES )
    # For Visual Studio add the header files and group files for easy access in IDE.
    if(MSVC)
        # The last regular expression that matches a file  applies.  Here 'moc_' is tighter than '.cc'.
        source_group(sources REGULAR_EXPRESSION "\\.cc")
        source_group(moc REGULAR_EXPRESSION "moc_")
        
        # The last regular expression that matches a file  applies. Here 'Ui.h' is tighter than '.h'.
        source_group(headers REGULAR_EXPRESSION "\\.h")
        source_group(uih REGULAR_EXPRESSION "Ui\\.h")
        
        source_group(ui REGULAR_EXPRESSION "\\.ui")
        source_group(qrc REGULAR_EXPRESSION "\\.qrc")
		
		source_group(pch REGULAR_EXPRESSION "_pch")
		source_group(rc REGULAR_EXPRESSION "\\.rc")
    endif(MSVC)
ENDMACRO (GPLATES_GROUP_SOURCE_FILES)

MACRO (GPLATES_GENERATE_TARGET_SOURCES _target_name _target_srcs )
    # Separate '.cc', '.h', '.ui' and '.qrc'.
    string(REGEX MATCHALL "[^ ;]+\\.cc" SRCS "${ARGN}")
    string(REGEX MATCHALL "[^ ;]+\\.h" HDRS "${ARGN}")
    string(REGEX MATCHALL "[^ ;]+\\.ui" UIS "${ARGN}")
    string(REGEX MATCHALL "[^ ;]+\\.qrc" QRCS "${ARGN}")
    if(MSVC)
		# '.rc' file used to embedd icon into GPlates executable.
		string(REGEX MATCHALL "[^ ;]+\\.rc" RCS "${ARGN}")
    endif(MSVC)

    # Add commands to generate 'Ui.h' files from '.ui' files.
    qt4_wrap_ui(UIS_H ${UIS})

    # Parse all the headers and return those that have "Q_OBJECT" in them (need to generate moc_*.cc files for these).
    gplates_get_moc_headers(MOC_HDRS ${HDRS})
    qt4_wrap_cpp(MOC_SRCS ${MOC_HDRS})

    #
    # We don't really need qt4_automoc() since we currently scan all headers for Q_OBJECT and moc those.
    #
    ## For 'qt4_automoc()' to do anything you need to, for example, put:
    ##	#include "AboutDialog.moc"
    ## in your AboutDialog.cc file and not pass AboutDialog.cc to qt4_wrap_cpp().
    ## This is because 'qt4_automoc()' scans for #include "*.moc" in the listed source files
    ## and adds a custom command to generate the ".moc" file.
    #qt4_automoc(${SRCS})

    # Compile '.qrc' files to '.cc'.
	# New version of FindQt4 package scans '.qrc' files for internal image files and adds dependencies to them.
    qt4_add_resources(RCC_SRCS ${QRCS})

    # List of source files to add to library.
    set(${_target_srcs} ${SRCS} ${MOC_SRCS} ${RCC_SRCS})

    # A better way to handle file dependencies, than using 'add_file_dependencies()', is to include
    # the headers generated from *.ui files in the list of sources required by the target.
    # This is only possible in CMake 2.4 and greater .
    # See http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_generate_a_source_file_during_the_build.3F
    set(${_target_srcs} ${${_target_srcs}} ${UIS_H})

	# Resource files for Visual Studio.
    if(MSVC)
		set(${_target_srcs} ${${_target_srcs}} ${RCS})
	endif(MSVC)
	
	# If user has chosen pre-compiled headers then add pch functionality.
if (GPLATES_USE_PCH)
	set(_pch_header "${_target_name}_pch.h")
	
	# Add pch header to list of header files.
	set(HDRS ${HDRS} "${_pch_header}")

	# If pch header doesn't currently exist in the source tree then create an empty header file.
	# This pch header needs to be updated outside the CMake system.
	if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_pch_header}")
		file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/${_pch_header}"
			"// Add frequently used but infrequently modified include headers here.\n\n")
	endif (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_pch_header}")

	if(CMAKE_GENERATOR MATCHES Visual* OR CMAKE_GENERATOR MATCHES Xcode)
		# Get the native pre-compiled header (generates '${_target_name}_pch.cc' if doesn't already exist)
		# and sets ${_target_name}_pch variable to it's filename).
		GET_NATIVE_PRECOMPILED_HEADER(${_target_name} ${_pch_header})

		# According to 'PCHSupport.cmake' only msvc needs to explicitly have a '*_pch.cc' file.
		# If '${_target_name}_pch' is defined then it's msvc we're generating.
		if (${_target_name}_pch)
			#  Add pre-compiled '.cc' file to list of target sources as requested by 'PCHSupport.cmake' module.
			# NOTE: this needs to be done before we add sources to the target (which is why this is not in the
			# GPLATES_POST_ADD_TARGET macro like the rest of the pch commands).
		    set(${_target_srcs} ${${_target_srcs}} "${${_target_name}_pch}")
		endif (${_target_name}_pch)
	endif(CMAKE_GENERATOR MATCHES Visual* OR CMAKE_GENERATOR MATCHES Xcode)
	
endif (GPLATES_USE_PCH)

    # For Visual Studio add the header files and group files for easy access in IDE.
    GPLATES_ADD_HEADERS_TO_SOURCE_FILES(${_target_srcs} ${HDRS})
    GPLATES_GROUP_SOURCE_FILES()

ENDMACRO (GPLATES_GENERATE_TARGET_SOURCES _target_name _target_srcs )

# Commands that need to be done after 'add_executable', 'add_library' (ie, commands that add a target).
# Pre-compiled headers are in this category because target properties are modified.
MACRO (GPLATES_POST_ADD_TARGET _target_name _target_srcs)

	# If user has chosen pre-compiled headers then add pch functionality.
	if (GPLATES_USE_PCH)
		set(_pch_header "${_target_name}_pch.h")
		
		if(CMAKE_GENERATOR MATCHES Visual* OR CMAKE_GENERATOR MATCHES Xcode)
			# Adds pch compile flags to all sources added to target (including the pre-compiled '.cc' file).
			ADD_NATIVE_PRECOMPILED_HEADER(${_target_name} ${_pch_header})
		else(CMAKE_GENERATOR MATCHES Visual* OR CMAKE_GENERATOR MATCHES Xcode)
			ADD_PRECOMPILED_HEADER(${_target_name} ${_pch_header} ${_target_srcs} 1 )
		endif(CMAKE_GENERATOR MATCHES Visual* OR CMAKE_GENERATOR MATCHES Xcode)
		
	endif (GPLATES_USE_PCH)
ENDMACRO (GPLATES_POST_ADD_TARGET _target_name )
