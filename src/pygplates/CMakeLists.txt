# Current binary directory must be included first for g++ pre-compiled headers.
# There's a '_pch.h' file in current source and current binary directory however the '_pch.h.gch' pre-compiled header
# is only in current binary directory and it will only be used by the compiler if it's in the same directory as the found '_pch.h' file.
# So we need to ensure that we find the '_pch.h' file from the current binary directory first.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# Current source directory (directory that this 'CMakeLists.txt' file is in).
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# The variables used here are set in the parent 'CMakeList.txt' file.
include_directories(${SYSTEM_INCLUDE_FLAG} ${external_INCLUDE_DIRS}) 
include_directories(${internal_INCLUDE_DIRS})

set(target_name pygplates)

# Does the following:
# - adds dependencies on generated header files.
# - finds '.ui' files and adds commands to create 'Ui.h' files.
# - scans '.h' files for Q_OBJECT and adds commands to run Qt's moc.exe to generate 'moc_*.cc' files.
# - finds '.qrc' files and adds commands to generate '.cc' files from them.
# - adds header files to Visual Studio projects.
# - groups files in Visual Studio IDE according to type.
if(NOT GPLATES_NO_PYTHON)

	set(all_subdirs qt-widgets qt-resources view-operations presentation gui canvas-tools app-logic file-io feature-visitors opengl property-values model data-mining api maths utils global scribe)
	
	#get all GPlates files
	foreach(dir ${all_subdirs})
		FILE(GLOB files ${GPlates_SOURCE_DIR}/src/${dir}/*)
		set(all_files ${all_files} ${files})
	endforeach(dir)
	
	# Add the scribe export registrations.
	set(all_files ${all_files} ScribeExportPyGPlates.cc)
	
	#generate moc,ui,pch,rc files...
	GPLATES_GENERATE_TARGET_SOURCES(pygplates gplates_lib_all_src ${all_files})
	
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${EXECUTABLE_OUTPUT_PATH}")
	add_library(pygplates MODULE ${gplates_lib_all_src})
    
	if(WIN32)
		set_target_properties(pygplates PROPERTIES SUFFIX ".pyd")
	else(WIN32)
	    set_target_properties(pygplates PROPERTIES COMPILE_FLAGS "-fPIC")
    endif(WIN32)

	set_target_properties(pygplates PROPERTIES PREFIX "")
	
	target_link_libraries(pygplates ${PYTHON_LIBRARIES} ${dependency_LIBS})
	
	GPLATES_POST_ADD_TARGET(${target_name} gplates_lib_all_src)
	
	install(TARGETS pygplates
		DESTINATION share/gplates/lib
		CONFIGURATIONS release)
	
endif(NOT GPLATES_NO_PYTHON)


