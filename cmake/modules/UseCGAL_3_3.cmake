#
# NOTE: This file is only used for CGAL versions 3.3 and below on linux systems.
# This is because MacOS and Windows should use CGAL 3.5 which has cmake support
# (versions 3.4 and above have cmake support).
# Some older linux systems (using the binary package management system) only support
# CGAL 3.3 which does not has cmake support.
# In that case this file is included when calling 'include(${CGAL_USE_FILE})'
# instead of the file in the CGAL installation. And this file uses some variables
# set in 'FindCGAL.cmake'.
#

if(NOT USE_CGAL_FILE_INCLUDED) 
  set(USE_CGAL_FILE_INCLUDED 1)

  # The 'include_directories' and 'link_libraries' is really just mirroring
  # to a certain extent what the 'UseCGAL.cmake' file does (for CGAL versions 3.4 and above).
  #
  # 'CGAL_INCLUDE_DIR' and 'CGAL_LIBRARY' were set in 'FindCGAL.cmake'.
  include_directories ( ${SYSTEM_INCLUDE_FLAG} ${CGAL_INCLUDE_DIR} )     
  link_libraries      ( ${CGAL_LIBRARY} )

  # Set the compiler flags that are required to match those used when building the CGAL library.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frounding-math -fno-strict-aliasing")
  
endif(NOT USE_CGAL_FILE_INCLUDED) 
