# Find Qwt
# --------
# Qt Widgets for Technical Applications (http://www.http://qwt.sourceforge.net/)
#
# Defines the following variables:
#  Qwt_FOUND - the system has Qwt
#  QWT_INCLUDE_DIR - where to find qwt_plot.h
#  QWT_INCLUDE_DIRS - qwt includes (same as QWT_INCLUDE_DIR)
#  QWT_LIBRARY - where to find the Qwt library
#  QWT_LIBRARIES - aditional libraries (same as QWT_LIBRARY)
#  QWT_VERSION_STRING - version (eg. 6.1.4)
#  QWT_MAJOR_VERSION - major version (eg, 6)
#  QWT_MINOR_VERSION - maminorjor version (eg, 1)
#  QWT_PATCH_VERSION - patch version (eg, 4)
#
# Also defines imported target:
#  Qwt::Qwt

find_path(QWT_INCLUDE_DIR
  NAMES qwt_plot.h
  PATHS
  /usr/include
  /usr/local/include
  /opt/local/include
  # On Macports Qwt is installed into Qt5 (using "sudo port install qwt61 +qt5")...
  /opt/local/libexec/qt5/include
  "$ENV{QWT_INCLUDE_DIR}"
  "$ENV{INCLUDE}" 
  # The '-qt5' versions searched first (in case 'qwt' is a qt4 version)...
  PATH_SUFFIXES qwt-qt5 qwt6-qt5 qwt qwt6
)

set (QWT_INCLUDE_DIRS ${QWT_INCLUDE_DIR})

find_library(QWT_LIBRARY
  # The '-qt5' versions searched first (in case 'qwt' is a qt4 version)...
  NAMES qwt-qt5 qwt6-qt5 qwt qwt6
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    # On Macports Qwt is installed into Qt5 (using "sudo port install qwt61 +qt5")...
    /opt/local/libexec/qt5/lib
    "$ENV{QWT_LIB_DIR}"
)

set (QWT_LIBRARIES ${QWT_LIBRARY})

# version
set ( _VERSION_FILE ${QWT_INCLUDE_DIR}/qwt_global.h )
if ( EXISTS ${_VERSION_FILE} )
  file ( STRINGS ${_VERSION_FILE} _VERSION_LINE REGEX "define[ ]+QWT_VERSION_STR" )
  if ( _VERSION_LINE )
    string ( REGEX REPLACE ".*define[ ]+QWT_VERSION_STR[ ]+\"(.*)\".*" "\\1" QWT_VERSION_STRING "${_VERSION_LINE}" )
    # Remove anything except digits and dots. Some version strings are of the form "6.0.0-svn" for example.
    string ( REGEX MATCH "[0-9.,]*" QWT_VERSION_STRING "${QWT_VERSION_STRING}")
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1" QWT_MAJOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2" QWT_MINOR_VERSION "${QWT_VERSION_STRING}" )
    string ( REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3" QWT_PATCH_VERSION "${QWT_VERSION_STRING}" )
  endif ()
endif ()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qwt REQUIRED_VARS QWT_LIBRARY QWT_INCLUDE_DIR VERSION_VAR QWT_VERSION_STRING)

if (Qwt_FOUND AND NOT TARGET Qwt::Qwt)
	add_library(Qwt::Qwt IMPORTED INTERFACE)
	set_target_properties(Qwt::Qwt PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${QWT_INCLUDE_DIRS}")
	set_target_properties(Qwt::Qwt PROPERTIES INTERFACE_LINK_LIBRARIES "${QWT_LIBRARIES}")
endif ()

mark_as_advanced(QWT_LIBRARY QWT_INCLUDE_DIR)
