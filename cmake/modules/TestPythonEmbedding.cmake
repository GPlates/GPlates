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
	"int main() {\n"
	"	try {\n"
	"		Py_Initialize();\n"
	"		object main_module(handle<>(borrowed(PyImport_AddModule(\"__main__\"))));\n"
	"		object main_namespace = main_module.attr(\"__dict__\");\n"
	"		handle<> ignored(PyRun_String(\"print 1 + 2\", Py_file_input, main_namespace.ptr(), main_namespace.ptr()));\n"
	"	} catch (...) {\n"
	"		PyErr_Print();\n"
	"		return 1;\n"
	"	}\n"
	"	return 0;\n"
	"}\n")
SET(python_embedding_LIBS
	${Boost_LIBRARIES}
	${PYTHON_LIBRARIES})
SET(python_embedding_INCLUDE_DIRS
	${Boost_INCLUDE_DIR}
	${PYTHON_INCLUDE_PATH})
TRY_RUN(PYTHON_EMBEDDING_RUNS PYTHON_EMBEDDING_COMPILES
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_python_embedding.cc
	CMAKE_FLAGS "-DLINK_LIBRARIES:STRING=${python_embedding_LIBS}" "-DINCLUDE_DIRECTORIES=${python_embedding_INCLUDE_DIRS}"
	COMPILE_OUTPUT_VARIABLE PYTHON_EMBEDDING_COMPILE_OUTPUT
	RUN_OUTPUT_VARIABLE PYTHON_EMBEDDING_RUN_OUTPUT)

IF(NOT PYTHON_EMBEDDING_COMPILES)
	MESSAGE(STATUS "Python embedding test program compile output:")
	MESSAGE("${PYTHON_EMBEDDING_COMPILE_OUTPUT}")
	MESSAGE(FATAL_ERROR "Python embedding test program could not be compiled.")
ENDIF(NOT PYTHON_EMBEDDING_COMPILES)

IF(PYTHON_EMBEDDING_RUNS EQUAL 1) # Program above returns 1 if exception thrown.
	MESSAGE(STATUS "Python embedding test program output: ${PYTHON_EMBEDDING_RUN_OUTPUT}")
	MESSAGE(FATAL_ERROR "Python embedding test program failed to run correctly. "
		"If you have multiple Python installations, make sure that the version of "
		"Python that GPlates is being linked against is the same version of Python "
		"that Boost.Python was built against. In particular, on Mac OS X, if "
		"Boost.Python was installed using MacPorts, provide the following CMake flag: "
		"-DCMAKE_FRAMEWORK_PATH=/opt/local/Library/Frameworks")
ENDIF(PYTHON_EMBEDDING_RUNS EQUAL 1)

MESSAGE(STATUS "Python embedding test passed.")
