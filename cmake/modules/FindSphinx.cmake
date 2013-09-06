# This source was obtained from:
#   http://ericscottbarr.com/blog/2012/03/sphinx-and-cmake-beautiful-documentation-for-c-projects/
#
# Note that, at the time the above post, there was no official FindSphinx.cmake.
# As of cmake 2.8.11 it looks like there is still no FindSphinx.cmake, but this one might be
# a good source if you want to update this file:
#   http://www.rad.upenn.edu/sbia/software/basis/apidoc/latest/FindSphinx_8cmake_source.html

find_program(SPHINX_EXECUTABLE NAMES sphinx-build
  HINTS
  $ENV{SPHINX_DIR}
  PATH_SUFFIXES bin
  DOC "Sphinx documentation generator"
)
 
include(FindPackageHandleStandardArgs)
 
find_package_handle_standard_args(Sphinx DEFAULT_MSG
  SPHINX_EXECUTABLE
)
 
mark_as_advanced(
  SPHINX_EXECUTABLE
)