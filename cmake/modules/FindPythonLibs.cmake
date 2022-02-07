# This script was copied from CMake 2.6.2, with modifications.

# - Find python libraries
# This module finds if Python is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#
#  PYTHONLIBS_FOUND     = have the Python libs been found
#  PYTHON_LIBRARIES     = path to the python library
#  PYTHON_INCLUDE_PATH  = path to where Python.h is found
#  PYTHON_DEBUG_LIBRARIES = path to the debug library
#

# GPlates addition:
# Always recompute these variables in case the user has changed the
# CMAKE_FRAMEWORK_PATH variable.
# Note that UNSET was not introduced into CMake until 2.6.3.
SET(PYTHON_INCLUDE_PATH NOTFOUND CACHE FLEPATH "Python Header Files" FORCE)
SET(PYTHON_DEBUG_LIBRARY NOTFOUND CACHE FLEPATH "Python Framework" FORCE)
SET(PYTHON_LIBRARY NOTFOUND CACHE FLEPATH "Python Framework" FORCE)
# End GPlates addition.

# GPlates modification:
# Use the GPlates version of FindFrameworks.
INCLUDE(GPlatesFindFrameworks)

IF(APPLE)

# Search for the python framework on Apple.
GPLATES_FIND_FRAMEWORKS(Python)
MESSAGE(STATUS "python framework: ${Python_FRAMEWORKS}")
MESSAGE(STATUS "boost python library: ${Boost_PYTHON_LIBRARY}")

find_program (OTOOL otool)
 
execute_process (COMMAND
  ${OTOOL} -L ${Boost_PYTHON_LIBRARY}
  OUTPUT_VARIABLE _otool_output
  OUTPUT_STRIP_TRAILING_WHITESPACE)
 
string (REPLACE "\n" ";" _otool_output ${_otool_output})
 
foreach (_line ${_otool_output})
  if (_line MATCHES Python.framework)
    string (REGEX REPLACE "/Python[ (]+[^)]+[)]$" "" PYTHON_FRAMEWORK_ROOT ${_line})
  endif (_line MATCHES Python.framework) 
endforeach (_line)
 
if (PYTHON_FRAMEWORK_ROOT)
  string (STRIP ${PYTHON_FRAMEWORK_ROOT} PYTHON_FRAMEWORK_ROOT) 
  string (REGEX REPLACE "^.+/" "" PYTHON_FRAMEWORK_VERSION ${PYTHON_FRAMEWORK_ROOT})

  MESSAGE(STATUS "python framework root: ${PYTHON_FRAMEWORK_ROOT}")
  MESSAGE(STATUS "python framework version: ${PYTHON_FRAMEWORK_VERSION}")

  #SET(Python_FRAMEWORKS ${PYTHON_FRAMEWORK_ROOT} "")
  #MESSAGE(STATUS "python framework: ${Python_FRAMEWORKS}")
endif (PYTHON_FRAMEWORK_ROOT)
ENDIF(APPLE)

# End GPlates modification.

if(NOT GPLATES_PYTHON_3)
    FOREACH(_CURRENT_VERSION ${PYTHON_FRAMEWORK_VERSION} 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0 1.6 1.5)
else(NOT GPLATES_PYTHON_3)
    FOREACH(_CURRENT_VERSION ${PYTHON_FRAMEWORK_VERSION} 3.6 3.5 3.4)
endif(NOT GPLATES_PYTHON_3)

  STRING(REPLACE "." "" _CURRENT_VERSION_NO_DOTS ${_CURRENT_VERSION})
  IF(WIN32)
    FIND_LIBRARY(PYTHON_DEBUG_LIBRARY
      NAMES python${_CURRENT_VERSION_NO_DOTS}_d python
      PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs )
  ENDIF(WIN32)

  # GPlates addition:
  # Attempt to point PYTHON_LIBRARY at file in frameworks directory.
  # This change is necessary because we don't use -framework as in the original
  # script (see comments below).
  SET(PYTHON_FRAMEWORK_LIBRARIES)
  IF(Python_FRAMEWORKS AND NOT PYTHON_LIBRARY)
    FOREACH(dir ${Python_FRAMEWORKS})
      SET(PYTHON_FRAMEWORK_LIBRARIES ${PYTHON_FRAMEWORK_LIBRARIES}
      	${dir}/Versions/${_CURRENT_VERSION}/lib/python${_CURRENT_VERSION}/config)
    ENDFOREACH(dir)
  ENDIF(Python_FRAMEWORKS AND NOT PYTHON_LIBRARY)
  # End GPlates addition.

  FIND_LIBRARY(PYTHON_LIBRARY
    NAMES python${_CURRENT_VERSION_NO_DOTS} python${_CURRENT_VERSION}
    PATHS
      ${PYTHON_FRAMEWORK_LIBRARIES} # GPlates addition.
	  # GPlates addition: 
      # Comment out explicit registry paths (and avoid default paths) on first try in case CMAKE_LIBRARY_PATH
	  # environment variable is set by a GPlates developer (eg, "C:\SDK\python\Python-2.6.6\").
	  # Avoids finding something in "C:/Python26/ArcGIS10.0/libs/" for example.
	  # We do another search later that includes these paths though.
	  #
      # [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
    PATH_SUFFIXES
      python${_CURRENT_VERSION}/config
    NO_DEFAULT_PATH # GPlates modification.
  )

  SET(PYTHON_FRAMEWORK_INCLUDES)
  IF(Python_FRAMEWORKS AND NOT PYTHON_INCLUDE_PATH)
    FOREACH(dir ${Python_FRAMEWORKS})
      SET(PYTHON_FRAMEWORK_INCLUDES ${PYTHON_FRAMEWORK_INCLUDES}
        ${dir}/Versions/${_CURRENT_VERSION}/include/python${_CURRENT_VERSION})
    ENDFOREACH(dir)
  ENDIF(Python_FRAMEWORKS AND NOT PYTHON_INCLUDE_PATH)

  FIND_PATH(PYTHON_INCLUDE_PATH
    NAMES Python.h
    PATHS
      ${PYTHON_FRAMEWORK_INCLUDES}
	  # GPlates addition: 
      # Comment out explicit registry paths (and avoid default paths) on first try in case CMAKE_INCLUDE_PATH
	  # environment variable is set by a GPlates developer (eg, "C:\SDK\python\Python-2.6.6\Include\").
	  # Avoids finding something in "C:/Python26/ArcGIS10.0/include/" for example.
	  # We do another search later that includes these paths though.
	  #
      # [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
    PATH_SUFFIXES
      python${_CURRENT_VERSION}
    NO_DEFAULT_PATH # GPlates addition.
  )
  
ENDFOREACH(_CURRENT_VERSION)

# GPlates addition.
# Since we specified NO_DEFAULT_PATH above, we now search the default paths here
# which are only searched if the variables haven't already been set.
if(NOT GPLATES_PYTHON_3)
    FOREACH(_CURRENT_VERSION ${PYTHON_FRAMEWORK_VERSION} 2.7 2.6 2.5 2.4 2.3 2.2 2.1 2.0 1.6 1.5)
else(NOT GPLATES_PYTHON_3)
    FOREACH(_CURRENT_VERSION ${PYTHON_FRAMEWORK_VERSION} 3.6 3.5 3.4)
endif(NOT GPLATES_PYTHON_3)

  STRING(REPLACE "." "" _CURRENT_VERSION_NO_DOTS ${_CURRENT_VERSION})

  FIND_LIBRARY(PYTHON_LIBRARY
    NAMES python${_CURRENT_VERSION_NO_DOTS} python${_CURRENT_VERSION} python${_CURRENT_VERSION}m
	# We avoided searching the registry above so do it now.
	PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
    PATH_SUFFIXES
      python${_CURRENT_VERSION}/config
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
  )

  FIND_PATH(PYTHON_INCLUDE_PATH
    NAMES Python.h
	# We avoided searching the registry above so do it now.
	PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
    PATH_SUFFIXES
      python${_CURRENT_VERSION}
  )
ENDFOREACH(_CURRENT_VERSION)
# End GPlates addition.

MARK_AS_ADVANCED(
  PYTHON_DEBUG_LIBRARY
  PYTHON_LIBRARY
  PYTHON_INCLUDE_PATH
)

# GPlates modification.
# The whole block of code that sets PYTHON_LIBRARY and PYTHON_DEBUG_LIBRARY to
# "-framework Python" if Python is installed as a framework on Darwin was
# removed. As a consequence, we always link to the Python libraries like any
# other non-framework library. This is because if we want to use the MacPorts
# version of Python (because Boost.Python got compiled against that version),
# specifying "-framework Python" would cause GPlates to be built against the
# system version of Python (thus causing a version mismatch). Specifying the -F
# flag to g++ (adding framework search paths) appears to work on some systems
# but not on others unfortunately.

# We use PYTHON_LIBRARY and PYTHON_DEBUG_LIBRARY for the cache entries
# because they are meant to specify the location of a single library.
# We now set the variables listed by the documentation for this
# module.
SET(PYTHON_LIBRARIES "${PYTHON_LIBRARY}")
SET(PYTHON_DEBUG_LIBRARIES "${PYTHON_DEBUG_LIBRARY}")


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PythonLibs DEFAULT_MSG PYTHON_LIBRARIES PYTHON_INCLUDE_PATH)


# PYTHON_ADD_MODULE(<name> src1 src2 ... srcN) is used to build modules for python.
# PYTHON_WRITE_MODULES_HEADER(<filename>) writes a header file you can include 
# in your sources to initialize the static python modules

GET_PROPERTY(_TARGET_SUPPORTS_SHARED_LIBS
  GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)

FUNCTION(PYTHON_ADD_MODULE _NAME )
  OPTION(PYTHON_ENABLE_MODULE_${_NAME} "Add module ${_NAME}" TRUE)
  OPTION(PYTHON_MODULE_${_NAME}_BUILD_SHARED "Add module ${_NAME} shared" ${_TARGET_SUPPORTS_SHARED_LIBS})

  IF(PYTHON_ENABLE_MODULE_${_NAME})
    IF(PYTHON_MODULE_${_NAME}_BUILD_SHARED)
      SET(PY_MODULE_TYPE MODULE)
    ELSE(PYTHON_MODULE_${_NAME}_BUILD_SHARED)
      SET(PY_MODULE_TYPE STATIC)
      SET_PROPERTY(GLOBAL  APPEND  PROPERTY  PY_STATIC_MODULES_LIST ${_NAME})
    ENDIF(PYTHON_MODULE_${_NAME}_BUILD_SHARED)

    SET_PROPERTY(GLOBAL  APPEND  PROPERTY  PY_MODULES_LIST ${_NAME})
    ADD_LIBRARY(${_NAME} ${PY_MODULE_TYPE} ${ARGN})
#    TARGET_LINK_LIBRARIES(${_NAME} ${PYTHON_LIBRARIES})

  ENDIF(PYTHON_ENABLE_MODULE_${_NAME})
ENDFUNCTION(PYTHON_ADD_MODULE)

FUNCTION(PYTHON_WRITE_MODULES_HEADER _filename)

  GET_PROPERTY(PY_STATIC_MODULES_LIST  GLOBAL  PROPERTY PY_STATIC_MODULES_LIST)

  GET_FILENAME_COMPONENT(_name "${_filename}" NAME)
  STRING(REPLACE "." "_" _name "${_name}")
  STRING(TOUPPER ${_name} _name)

  SET(_filenameTmp "${_filename}.in")
  FILE(WRITE ${_filenameTmp} "/*Created by cmake, do not edit, changes will be lost*/\n")
  FILE(APPEND ${_filenameTmp} 
"#ifndef ${_name}
#define ${_name}

#include <Python.h>

#ifdef __cplusplus
extern \"C\" {
#endif /* __cplusplus */

")

  FOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
    FILE(APPEND ${_filenameTmp} "extern void init${PYTHON_MODULE_PREFIX}${_currentModule}(void);\n\n")
  ENDFOREACH(_currentModule ${PY_STATIC_MODULES_LIST})

  FILE(APPEND ${_filenameTmp} 
"#ifdef __cplusplus
}
#endif /* __cplusplus */

")


  FOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
    FILE(APPEND ${_filenameTmp} "int CMakeLoadPythonModule_${_currentModule}(void) \n{\n  static char name[]=\"${PYTHON_MODULE_PREFIX}${_currentModule}\"; return PyImport_AppendInittab(name, init${PYTHON_MODULE_PREFIX}${_currentModule});\n}\n\n")
  ENDFOREACH(_currentModule ${PY_STATIC_MODULES_LIST})

  FILE(APPEND ${_filenameTmp} "#ifndef EXCLUDE_LOAD_ALL_FUNCTION\nvoid CMakeLoadAllPythonModules(void)\n{\n")
  FOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
    FILE(APPEND ${_filenameTmp} "  CMakeLoadPythonModule_${_currentModule}();\n")
  ENDFOREACH(_currentModule ${PY_STATIC_MODULES_LIST})
  FILE(APPEND ${_filenameTmp} "}\n#endif\n\n#endif\n")
  
# with CONFIGURE_FILE() cmake complains that you may not use a file created using FILE(WRITE) as input file for CONFIGURE_FILE()
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_filenameTmp}" "${_filename}" OUTPUT_QUIET ERROR_QUIET)

ENDFUNCTION(PYTHON_WRITE_MODULES_HEADER)
