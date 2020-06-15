# Copyright (C) 2010 The University of Sydney, Australia
#
# This file is part of GPlates.
#
# GPlates is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation.
#
# GPlates is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

# Attempts to compile and run a simple C++ program that embeds the Python
# interpreter. This example code is known to fail if the Python library that we
# link against is different to the Python library used to build Boost.Python.
# This problem manifests itself especially on Mac OS X, where there is the
# system version of Python and the version of Python installed by Macports, if
# Boost is installed via Macports.

FILE(WRITE ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_python_embedding.cc
	"#include <boost/python.hpp>\n"
	"using namespace boost::python;\n"
	"struct TestStruct {  };\n"
	"BOOST_PYTHON_MODULE(testmodule) {\n"
	"	class_<TestStruct>(\"TestStruct\");\n"
	"}\n"
	"int main() {\n"
	"	try {\n"
	"		char MODULE_NAME[] = \"testmodule\";\n"
	"		#if PY_MAJOR_VERSION >= 3\n"
	"			PyImport_AppendInittab(MODULE_NAME, &PyInit_testmodule);\n"
	"		#else\n"
	"			PyImport_AppendInittab(MODULE_NAME, &inittestmodule);\n"
	"		#endif\n"
	"		Py_Initialize();\n"
	"		object main_module(handle<>(borrowed(PyImport_AddModule(\"__main__\"))));\n"
	"		object main_namespace = main_module.attr(\"__dict__\");\n"
	"		handle<> ignored(PyRun_String(\"import testmodule; t = testmodule.TestStruct()\", Py_file_input, main_namespace.ptr(), main_namespace.ptr()));\n"
	"	} catch (...) {\n"
	"		PyErr_Print();\n"
	"		return 1;\n"
	"	}\n"
	"	return 0;\n"
	"}\n")

SET(python_embedding_LIBS ${Boost_LIBRARIES})
SET(python_embedding_LIB_DIRS ${Boost_LIBRARY_DIRS})
SET(python_embedding_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
if (TARGET Python3::Python)
	# We used the Python3 find module.
	list(APPEND python_embedding_LIBS ${Python3_LIBRARIES})
	list(APPEND python_embedding_LIB_DIRS ${Python3_LIBRARY_DIRS})
	list(APPEND python_embedding_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
elseif (TARGET Python2::Python)
	# We used the Python2 find module.
	list(APPEND python_embedding_LIBS ${Python2_LIBRARIES})
	list(APPEND python_embedding_LIB_DIRS ${Python2_LIBRARY_DIRS})
	list(APPEND python_embedding_INCLUDE_DIRS ${Python2_INCLUDE_DIRS})
else()
	# We used the PythonLibs find module (which does not have imported targets).
	list(APPEND python_embedding_LIBS ${PYTHON_LIBRARIES})
	list(APPEND python_embedding_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
endif()

# According to the docs...
# Projects built by try_compile() and try_run() are built synchronously during the CMake configuration step.
# Therefore a specific build configuration must be chosen even if the generated build system supports multiple configurations.
SET(CMAKE_TRY_COMPILE_CONFIGURATION Release)
TRY_RUN(PYTHON_EMBEDDING_RUNS PYTHON_EMBEDDING_COMPILES
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_python_embedding.cc
	CMAKE_FLAGS "-DLINK_DIRECTORIES:STRING=${python_embedding_LIB_DIRS}" "-DINCLUDE_DIRECTORIES=${python_embedding_INCLUDE_DIRS}"
	LINK_LIBRARIES ${python_embedding_LIBS}
	COMPILE_OUTPUT_VARIABLE PYTHON_EMBEDDING_COMPILE_OUTPUT
	RUN_OUTPUT_VARIABLE PYTHON_EMBEDDING_RUN_OUTPUT)

IF(NOT PYTHON_EMBEDDING_COMPILES)
	MESSAGE(STATUS "Python embedding test program compile output:")
	MESSAGE("${PYTHON_EMBEDDING_COMPILE_OUTPUT}")
	MESSAGE(FATAL_ERROR "Python embedding test program could not be compiled.")
ENDIF(NOT PYTHON_EMBEDDING_COMPILES)

IF(PYTHON_EMBEDDING_RUNS EQUAL 1 # Program above returns 1 if exception thrown.
		OR PYTHON_EMBEDDING_RUNS STREQUAL FAILED_TO_RUN # The program crashed.
		)
	MESSAGE(STATUS "Python embedding test program output: ${PYTHON_EMBEDDING_RUN_OUTPUT}")
	MESSAGE(FATAL_ERROR "Python embedding test program failed to run correctly. "
		"Check that the Python and Boost.Python libraries are in your system path."
		"If you have multiple Python installations, make sure that the version of "
		"Python that GPlates is being linked against is the same version of Python "
		"that Boost.Python was built against. In particular, on Mac OS X, if "
		"Boost.Python was installed using MacPorts, provide the following CMake flag: "
		"-DCMAKE_FRAMEWORK_PATH=/opt/local/Library/Frameworks")
ENDIF(PYTHON_EMBEDDING_RUNS EQUAL 1 OR PYTHON_EMBEDDING_RUNS STREQUAL FAILED_TO_RUN)

MESSAGE(STATUS "Python embedding test passed.")
