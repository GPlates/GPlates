# - Find QT 4
# This module can be used to find Qt4.
# The most important issue is that the Qt4 qmake is available via the system path.
# This qmake is then used to detect basically everything else.
# This module defines a number of key variables and macros. First is 
# QT_USE_FILE which is the path to a CMake file that can be included to compile
# Qt 4 applications and libraries.  By default, the QtCore and QtGui 
# libraries are loaded. This behavior can be changed by setting one or more 
# of the following variables to true:
#                    QT_DONT_USE_QTCORE
#                    QT_DONT_USE_QTGUI
#                    QT_USE_QT3SUPPORT
#                    QT_USE_QTASSISTANT
#                    QT_USE_QTDESIGNER
#                    QT_USE_QTMOTIF
#                    QT_USE_QTMAIN
#                    QT_USE_QTNETWORK
#                    QT_USE_QTNSPLUGIN
#                    QT_USE_QTOPENGL
#                    QT_USE_QTSQL
#                    QT_USE_QTXML
#                    QT_USE_QTXMLPATTERNS
#                    QT_USE_QTSVG
#                    QT_USE_QTTEST
#                    QT_USE_QTUITOOLS
#
# All the libraries required are stored in a variable called QT_LIBRARIES.  
# Add this variable to your TARGET_LINK_LIBRARIES.
#  
#  macro QT4_WRAP_CPP(outfiles inputfile ... )
#  macro QT4_WRAP_UI(outfiles inputfile ... )
#  macro QT4_ADD_RESOURCE(outfiles inputfile ... )
#  macro QT4_AUTOMOC(inputfile ... )
#  macro QT4_GENERATE_MOC(inputfile outputfile )
#  
#  QT_FOUND         If false, don't try to use Qt.
#  QT4_FOUND        If false, don't try to use Qt 4.
#                      
#  QT_QTCORE_FOUND        True if QtCore was found.
#  QT_QTGUI_FOUND         True if QtGui was found.
#  QT_QT3SUPPORT_FOUND    True if Qt3Support was found.
#  QT_QTASSISTANT_FOUND   True if QtAssistant was found.
#  QT_QTDESIGNER_FOUND    True if QtDesigner was found.
#  QT_QTMOTIF_FOUND       True if QtMotif was found.
#  QT_QTNETWORK_FOUND     True if QtNetwork was found.
#  QT_QTNSPLUGIN_FOUND    True if QtNsPlugin was found.
#  QT_QTOPENGL_FOUND      True if QtOpenGL was found.
#  QT_QTSQL_FOUND         True if QtSql was found.
#  QT_QTXML_FOUND         True if QtXml was found.
#  QT_QTXMLPATTERNS_FOUND True if QtXmlPatterns was found.
#  QT_QTSVG_FOUND         True if QtSvg was found.
#  QT_QTTEST_FOUND        True if QtTest was found.
#  QT_QTUITOOLS_FOUND     True if QtUiTools was found.
#                      
#  QT_DEFINITIONS   Definitions to use when compiling code that uses Qt.
#                  
#  QT_INCLUDES      List of paths to all include directories of 
#                   Qt4 QT_INCLUDE_DIR and QT_QTCORE_INCLUDE_DIR are
#                   always in this variable even if NOTFOUND,
#                   all other INCLUDE_DIRS are
#                   only added if they are found.
#   
#  QT_INCLUDE_DIR              Path to "include" of Qt4
#  QT_QT_INCLUDE_DIR           Path to "include/Qt" 
#  QT_QT3SUPPORT_INCLUDE_DIR   Path to "include/Qt3Support" 
#  QT_QTASSISTANT_INCLUDE_DIR  Path to "include/QtAssistant" 
#  QT_QTCORE_INCLUDE_DIR       Path to "include/QtCore"         
#  QT_QTDESIGNER_INCLUDE_DIR   Path to "include/QtDesigner" 
#  QT_QTGUI_INCLUDE_DIR        Path to "include/QtGui" 
#  QT_QTMOTIF_INCLUDE_DIR      Path to "include/QtMotif" 
#  QT_QTNETWORK_INCLUDE_DIR    Path to "include/QtNetwork" 
#  QT_QTNSPLUGIN_INCLUDE_DIR   Path to "include/QtNsPlugin" 
#  QT_QTOPENGL_INCLUDE_DIR     Path to "include/QtOpenGL" 
#  QT_QTSQL_INCLUDE_DIR        Path to "include/QtSql" 
#  QT_QTXML_INCLUDE_DIR        Path to "include/QtXml" 
#  QT_QTXMLPATTERNS_INCLUDE_DIR  Path to "include/QtXmlPatterns
#  QT_QTSVG_INCLUDE_DIR        Path to "include/QtSvg"
#  QT_QTTEST_INCLUDE_DIR       Path to "include/QtTest"
#                            
#  QT_LIBRARY_DIR              Path to "lib" of Qt4
#                            
#  QT_PLUGINS_DIR              Path to "plugins" for Qt4
#                            
# For every library of Qt there are three variables:
#  QT_QTFOO_LIBRARY_RELEASE, which contains the full path to the release version
#  QT_QTFOO_LIBRARY_DEBUG, which contains the full path to the debug version
#  QT_QTFOO_LIBRARY, the full path to the release version if available, otherwise to the debug version
#
# So there are the following variables:
# The Qt3Support library:     QT_QT3SUPPORT_LIBRARY
#                             QT_QT3SUPPORT_LIBRARY_RELEASE
#                             QT_QT3SUPPORT_DEBUG
#
# The QtAssistant library:    QT_QTASSISTANT_LIBRARY
#                             QT_QTASSISTANT_LIBRARY_RELEASE
#                             QT_QTASSISTANT_LIBRARY_DEBUG
#
# The QtCore library:         QT_QTCORE_LIBRARY
#                             QT_QTCORE_LIBRARY_RELEASE
#                             QT_QTCORE_LIBRARY_DEBUG
#
# The QtDesigner library:     QT_QTDESIGNER_LIBRARY
#                             QT_QTDESIGNER_LIBRARY_RELEASE
#                             QT_QTDESIGNER_LIBRARY_DEBUG
#
# The QtGui library:          QT_QTGUI_LIBRARY
#                             QT_QTGUI_LIBRARY_RELEASE
#                             QT_QTGUI_LIBRARY_DEBUG
#
# The QtMotif library:        QT_QTMOTIF_LIBRARY
#                             QT_QTMOTIF_LIBRARY_RELEASE
#                             QT_QTMOTIF_LIBRARY_DEBUG
#
# The QtNetwork library:      QT_QTNETWORK_LIBRARY
#                             QT_QTNETWORK_LIBRARY_RELEASE
#                             QT_QTNETWORK_LIBRARY_DEBUG
#
# The QtNsPLugin library:     QT_QTNSPLUGIN_LIBRARY
#                             QT_QTNSPLUGIN_LIBRARY_RELEASE
#                             QT_QTNSPLUGIN_LIBRARY_DEBUG
#
# The QtOpenGL library:       QT_QTOPENGL_LIBRARY
#                             QT_QTOPENGL_LIBRARY_RELEASE
#                             QT_QTOPENGL_LIBRARY_DEBUG
#
# The QtSql library:          QT_QTSQL_LIBRARY
#                             QT_QTSQL_LIBRARY_RELEASE
#                             QT_QTSQL_LIBRARY_DEBUG
#
# The QtXml library:          QT_QTXML_LIBRARY
#                             QT_QTXML_LIBRARY_RELEASE
#                             QT_QTXML_LIBRARY_DEBUG
# The QtXmlPatterns library:  QT_QTXMLPATTERNS_LIBRARY
#                             QT_QTXMLPATTERNS_LIBRARY_RELEASE
#                             QT_QTXMLPATTERNS_LIBRARY_DEBUG
#
# The QtSvg library:          QT_QTSVG_LIBRARY
#                             QT_QTSVG_LIBRARY_RELEASE
#                             QT_QTSVG_LIBRARY_DEBUG
#
# The QtTest library:         QT_QTTEST_LIBRARY
#                             QT_QTTEST_LIBRARY_RELEASE
#                             QT_QTTEST_LIBRARY_DEBUG
#
# The qtmain library for Windows QT_QTMAIN_LIBRARY
#                             QT_QTMAIN_LIBRARY_RELEASE
#                             QT_QTMAIN_LIBRARY_DEBUG
#
#The QtUiTools library:       QT_QTUITOOLS_LIBRARY
#                             QT_QTUITOOLS_LIBRARY_RELEASE
#                             QT_QTUITOOLS_LIBRARY_DEBUG
#  
# also defined, but NOT for general use are
#  QT_MOC_EXECUTABLE          Where to find the moc tool.
#  QT_UIC_EXECUTABLE          Where to find the uic tool.
#  QT_UIC3_EXECUTABLE         Where to find the uic3 tool.
#  QT_RCC_EXECUTABLE          Where to find the rcc tool
#  
#  QT_DOC_DIR                 Path to "doc" of Qt4
#  QT_MKSPECS_DIR             Path to "mkspecs" of Qt4
#
#
# These are around for backwards compatibility 
# they will be set
#  QT_WRAP_CPP  Set true if QT_MOC_EXECUTABLE is found
#  QT_WRAP_UI   Set true if QT_UIC_EXECUTABLE is found
#  
# These variables do _NOT_ have any effect anymore (compared to FindQt.cmake)
#  QT_MT_REQUIRED         Qt4 is now always multithreaded
#  
# These variables are set to "" Because Qt structure changed 
# (They make no sense in Qt4)
#  QT_QT_LIBRARY        Qt-Library is now split

INCLUDE(CheckSymbolExists)
INCLUDE(MacroAddFileDependencies)

SET(QT_USE_FILE UseQt4)

SET( QT_DEFINITIONS "")

# check for qmake
FIND_PROGRAM(QT_QMAKE_EXECUTABLE NAMES qmake qmake-qt4 PATHS
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\4.0.0;InstallDir]/bin"
  "[HKEY_CURRENT_USER\\Software\\Trolltech\\Versions\\4.0.0;InstallDir]/bin"
  $ENV{QTDIR}/bin
  )

SET(QT4_INSTALLED_VERSION_TOO_OLD FALSE)

##  macro for asking qmake to process pro files
MACRO(QT_QUERY_QMAKE outvar invar)
  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmpQmake/tmp.pro
    "message(CMAKE_MESSAGE<$$${invar}>)")
  EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE}
    WORKING_DIRECTORY  
    ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmpQmake
    OUTPUT_VARIABLE _qmake_query_output
    ERROR_VARIABLE _qmake_query_output
	OUTPUT_STRIP_TRAILING_WHITESPACE)
  FILE(REMOVE_RECURSE 
    "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmpQmake")
  STRING(REGEX REPLACE ".*CMAKE_MESSAGE<([^>]*).*" "\\1" ${outvar} "${_qmake_query_output}")
ENDMACRO(QT_QUERY_QMAKE)


IF (QT_QMAKE_EXECUTABLE)

   SET(QT4_QMAKE_FOUND FALSE)
   
   EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QTVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  # check for qt3 qmake and then try and find qmake-qt4 in the path
  IF("${QTVERSION}" MATCHES "Unknown")
    SET(QT_QMAKE_EXECUTABLE NOTFOUND CACHE FILEPATH "" FORCE)
    FIND_PROGRAM(QT_QMAKE_EXECUTABLE NAMES qmake-qt4 PATHS
      "[HKEY_CURRENT_USER\\Software\\Trolltech\\Qt3Versions\\4.0.0;InstallDir]/bin"
      "[HKEY_CURRENT_USER\\Software\\Trolltech\\Versions\\4.0.0;InstallDir]/bin"
      $ENV{QTDIR}/bin
      )
    IF(QT_QMAKE_EXECUTABLE)
      EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QTVERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF(QT_QMAKE_EXECUTABLE)
  ENDIF("${QTVERSION}" MATCHES "Unknown")
   # check that we found the Qt4 qmake, Qt3 qmake output won't match here
   STRING(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" qt_version_tmp "${QTVERSION}")
   IF (qt_version_tmp)

      # we need at least version 4.0.0
      IF (NOT QT_MIN_VERSION)
         SET(QT_MIN_VERSION "4.0.0")
      ENDIF (NOT QT_MIN_VERSION)
   
      #now parse the parts of the user given version string into variables
      STRING(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" req_qt_major_vers "${QT_MIN_VERSION}")
      IF (NOT req_qt_major_vers)
         MESSAGE( FATAL_ERROR "Invalid Qt version string given: \"${QT_MIN_VERSION}\", expected e.g. \"4.0.1\"")
      ENDIF (NOT req_qt_major_vers)
   
      # now parse the parts of the user given version string into variables
      STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" req_qt_major_vers "${QT_MIN_VERSION}")
      STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" req_qt_minor_vers "${QT_MIN_VERSION}")
      STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" req_qt_patch_vers "${QT_MIN_VERSION}")
   
      IF (NOT req_qt_major_vers EQUAL 4)
         MESSAGE( FATAL_ERROR "Invalid Qt version string given: \"${QT_MIN_VERSION}\", major version 4 is required, e.g. \"4.0.1\"")
      ENDIF (NOT req_qt_major_vers EQUAL 4)
   
      # and now the version string given by qmake
      STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" found_qt_major_vers "${QTVERSION}")
      STRING(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+.*" "\\1" found_qt_minor_vers "${QTVERSION}")
      STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" found_qt_patch_vers "${QTVERSION}")
   
      # compute an overall version number which can be compared at once
      MATH(EXPR req_vers "${req_qt_major_vers}*10000 + ${req_qt_minor_vers}*100 + ${req_qt_patch_vers}")
      MATH(EXPR found_vers "${found_qt_major_vers}*10000 + ${found_qt_minor_vers}*100 + ${found_qt_patch_vers}")
   
      IF (found_vers LESS req_vers)
         SET(QT4_QMAKE_FOUND FALSE)
         SET(QT4_INSTALLED_VERSION_TOO_OLD TRUE)
      ELSE (found_vers LESS req_vers)
    SET(QT4_QMAKE_FOUND TRUE)
      ENDIF (found_vers LESS req_vers)
   ENDIF (qt_version_tmp)

ENDIF (QT_QMAKE_EXECUTABLE)

IF (QT4_QMAKE_FOUND)

  # ask qmake for the library dir
  # Set QT_LIBRARY_DIR
  IF (NOT QT_LIBRARY_DIR)
    EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_LIBS OUTPUT_VARIABLE QT_LIBRARY_DIR_TMP OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(EXISTS "${QT_LIBRARY_DIR_TMP}")
      SET(QT_LIBRARY_DIR ${QT_LIBRARY_DIR_TMP} CACHE PATH "Qt library dir")
    ELSE(EXISTS "${QT_LIBRARY_DIR_TMP}")
      MESSAGE("Warning: QT_QMAKE_EXECUTABLE reported QT_INSTALL_LIBS as ${QT_LIBRARY_DIR_TMP}")
       MESSAGE("Warning: ${QT_LIBRARY_DIR_TMP} does NOT exist, Qt must NOT be installed correctly.")
    ENDIF(EXISTS "${QT_LIBRARY_DIR_TMP}")
  ENDIF(NOT QT_LIBRARY_DIR)
  
  IF (APPLE)
    IF (EXISTS ${QT_LIBRARY_DIR}/QtCore.framework)
      SET(QT_USE_FRAMEWORKS ON
        CACHE BOOL "Set to ON if Qt build uses frameworks.")
    ELSE (EXISTS ${QT_LIBRARY_DIR}/QtCore.framework)
      SET(QT_USE_FRAMEWORKS OFF
        CACHE BOOL "Set to ON if Qt build uses frameworks.")
    ENDIF (EXISTS ${QT_LIBRARY_DIR}/QtCore.framework)
    
    MARK_AS_ADVANCED(QT_USE_FRAMEWORKS)
  ENDIF (APPLE)
  
  # ask qmake for the binary dir
  IF (NOT QT_BINARY_DIR)
     EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_BINS OUTPUT_VARIABLE qt_bins OUTPUT_STRIP_TRAILING_WHITESPACE)
     SET(QT_BINARY_DIR ${qt_bins} CACHE INTERNAL "")
  ENDIF (NOT QT_BINARY_DIR)

  # ask qmake for the include dir
  IF (NOT QT_HEADERS_DIR)
      EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_HEADERS OUTPUT_VARIABLE qt_headers OUTPUT_STRIP_TRAILING_WHITESPACE)
      SET(QT_HEADERS_DIR ${qt_headers} CACHE INTERNAL "")
  ENDIF(NOT QT_HEADERS_DIR)


  # ask qmake for the documentation directory
  IF (NOT QT_DOC_DIR)
    EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_DOCS OUTPUT_VARIABLE qt_doc_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(QT_DOC_DIR ${qt_doc_dir} CACHE PATH "The location of the Qt docs")
  ENDIF (NOT QT_DOC_DIR)

  # ask qmake for the mkspecs directory
  IF (NOT QT_MKSPECS_DIR)
    EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QMAKE_MKSPECS OUTPUT_VARIABLE qt_mkspecs_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(QT_MKSPECS_DIR ${qt_mkspecs_dir} CACHE PATH "The location of the Qt mkspecs")
  ENDIF (NOT QT_MKSPECS_DIR)

  # ask qmake for the plugins directory
  IF (NOT QT_PLUGINS_DIR)
    EXECUTE_PROCESS(COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS OUTPUT_VARIABLE qt_plugins_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
    SET(QT_PLUGINS_DIR ${qt_plugins_dir} CACHE PATH "The location of the Qt plugins")
  ENDIF (NOT QT_PLUGINS_DIR)
  ########################################
  #
  #       Setting the INCLUDE-Variables
  #
  ########################################

  FIND_PATH(QT_QTCORE_INCLUDE_DIR QtGlobal
    ${QT_HEADERS_DIR}/QtCore
    ${QT_LIBRARY_DIR}/QtCore.framework/Headers
    NO_DEFAULT_PATH
   )

  # Set QT_INCLUDE_DIR by removine "/QtCore" in the string ${QT_QTCORE_INCLUDE_DIR}
  IF( QT_QTCORE_INCLUDE_DIR AND NOT QT_INCLUDE_DIR)
    IF (QT_USE_FRAMEWORKS)
      SET(QT_INCLUDE_DIR ${QT_HEADERS_DIR})
    ELSE (QT_USE_FRAMEWORKS)
      STRING( REGEX REPLACE "/QtCore$" "" qt4_include_dir ${QT_QTCORE_INCLUDE_DIR})
      SET( QT_INCLUDE_DIR ${qt4_include_dir} CACHE PATH "")
    ENDIF (QT_USE_FRAMEWORKS)
  ENDIF( QT_QTCORE_INCLUDE_DIR AND NOT QT_INCLUDE_DIR)

  IF( NOT QT_INCLUDE_DIR)
    IF( NOT Qt4_FIND_QUIETLY AND Qt4_FIND_REQUIRED)
      MESSAGE( FATAL_ERROR "Could NOT find QtGlobal header")
    ENDIF( NOT Qt4_FIND_QUIETLY AND Qt4_FIND_REQUIRED)
  ENDIF( NOT QT_INCLUDE_DIR)

  #############################################
  #
  # Find out what window system we're using
  #
  #############################################
  # Save required includes variable
  SET(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  # Add QT_INCLUDE_DIR to CMAKE_REQUIRED_INCLUDES
  SET(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${QT_INCLUDE_DIR}")
  # Check for Window system symbols (note: only one should end up being set)
  CHECK_SYMBOL_EXISTS(Q_WS_X11 "QtCore/qglobal.h" Q_WS_X11)
  CHECK_SYMBOL_EXISTS(Q_WS_MAC "QtCore/qglobal.h" Q_WS_MAC)
  CHECK_SYMBOL_EXISTS(Q_WS_WIN "QtCore/qglobal.h" Q_WS_WIN)

  IF (QT_QTCOPY_REQUIRED)
     CHECK_SYMBOL_EXISTS(QT_IS_QTCOPY "QtCore/qglobal.h" QT_KDE_QT_COPY)
     IF (NOT QT_IS_QTCOPY)
        MESSAGE(FATAL_ERROR "qt-copy is required, but hasn't been found")
     ENDIF (NOT QT_IS_QTCOPY)
  ENDIF (QT_QTCOPY_REQUIRED)

  # Restore CMAKE_REQUIRED_INCLUDES variable
  SET(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  #
  #############################################

  IF (QT_USE_FRAMEWORKS)
    SET(QT_DEFINITIONS ${QT_DEFINITIONS} -F${QT_LIBRARY_DIR} -L${QT_LIBRARY_DIR} )
  ENDIF (QT_USE_FRAMEWORKS)

  # Set QT_QT3SUPPORT_INCLUDE_DIR
  FIND_PATH(QT_QT3SUPPORT_INCLUDE_DIR Qt3Support
    PATHS
    ${QT_INCLUDE_DIR}/Qt3Support
    ${QT_LIBRARY_DIR}/Qt3Support.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QT_INCLUDE_DIR
  FIND_PATH(QT_QT_INCLUDE_DIR qglobal.h
    PATHS
    ${QT_INCLUDE_DIR}/Qt
    ${QT_LIBRARY_DIR}/QtCore.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTGUI_INCLUDE_DIR
  FIND_PATH(QT_QTGUI_INCLUDE_DIR QtGui
    PATHS
    ${QT_INCLUDE_DIR}/QtGui
    ${QT_LIBRARY_DIR}/QtGui.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTSVG_INCLUDE_DIR
  FIND_PATH(QT_QTSVG_INCLUDE_DIR QtSvg
    PATHS
    ${QT_INCLUDE_DIR}/QtSvg
    ${QT_LIBRARY_DIR}/QtSvg.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTTEST_INCLUDE_DIR
  FIND_PATH(QT_QTTEST_INCLUDE_DIR QtTest
    PATHS
    ${QT_INCLUDE_DIR}/QtTest
    ${QT_LIBRARY_DIR}/QtTest.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTUITOOLS_INCLUDE_DIR
  FIND_PATH(QT_QTUITOOLS_INCLUDE_DIR QtUiTools
    PATHS
    ${QT_INCLUDE_DIR}/QtUiTools
    ${QT_LIBRARY_DIR}/QtUiTools.framework/Headers
    NO_DEFAULT_PATH
    )



  # Set QT_QTMOTIF_INCLUDE_DIR
  IF(Q_WS_X11)
    FIND_PATH(QT_QTMOTIF_INCLUDE_DIR QtMotif PATHS ${QT_INCLUDE_DIR}/QtMotif NO_DEFAULT_PATH )
  ENDIF(Q_WS_X11)

  # Set QT_QTNETWORK_INCLUDE_DIR
  FIND_PATH(QT_QTNETWORK_INCLUDE_DIR QtNetwork
    PATHS
    ${QT_INCLUDE_DIR}/QtNetwork
    ${QT_LIBRARY_DIR}/QtNetwork.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTNSPLUGIN_INCLUDE_DIR
  FIND_PATH(QT_QTNSPLUGIN_INCLUDE_DIR QtNsPlugin
    PATHS
    ${QT_INCLUDE_DIR}/QtNsPlugin
    ${QT_LIBRARY_DIR}/QtNsPlugin.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTOPENGL_INCLUDE_DIR
  FIND_PATH(QT_QTOPENGL_INCLUDE_DIR QtOpenGL
    PATHS
    ${QT_INCLUDE_DIR}/QtOpenGL
    ${QT_LIBRARY_DIR}/QtOpenGL.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTSQL_INCLUDE_DIR
  FIND_PATH(QT_QTSQL_INCLUDE_DIR QtSql
    PATHS
    ${QT_INCLUDE_DIR}/QtSql
    ${QT_LIBRARY_DIR}/QtSql.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTXML_INCLUDE_DIR
  FIND_PATH(QT_QTXML_INCLUDE_DIR QtXml
    PATHS
    ${QT_INCLUDE_DIR}/QtXml
    ${QT_LIBRARY_DIR}/QtXml.framework/Headers
    NO_DEFAULT_PATH
    )
	
  # Set QT_QTXMLPATTERNS_INCLUDE_DIR
  FIND_PATH(QT_QTXMLPATTERNS_INCLUDE_DIR QtXmlPatterns
    PATHS
    ${QT_INCLUDE_DIR}/QtXmlPatterns
    ${QT_LIBRARY_DIR}/QtXmlPatterns.framework/Headers
    NO_DEFAULT_PATH
    )
	
  # Set QT_QTXMLPATTERNS_INCLUDE_DIR
  FIND_PATH(QT_QTNETWORK_INCLUDE_DIR QtNetwork
    PATHS
    ${QT_INCLUDE_DIR}/QtNetwork
    ${QT_LIBRARY_DIR}/QtNetwork.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTASSISTANT_INCLUDE_DIR
  FIND_PATH(QT_QTASSISTANT_INCLUDE_DIR QtAssistant
    PATHS
    ${QT_INCLUDE_DIR}/QtAssistant
    ${QT_HEADERS_DIR}/QtAssistant
    ${QT_LIBRARY_DIR}/QtAssistant.framework/Headers
    NO_DEFAULT_PATH
    )

  # Set QT_QTDESIGNER_INCLUDE_DIR
  FIND_PATH(QT_QTDESIGNER_INCLUDE_DIR QDesignerComponents
    PATHS
    ${QT_INCLUDE_DIR}/QtDesigner
    ${QT_HEADERS_DIR}/QtDesigner
    ${QT_LIBRARY_DIR}/QtDesigner.framework/Headers
    NO_DEFAULT_PATH
    )

  # Make variables changeble to the advanced user
  MARK_AS_ADVANCED( QT_LIBRARY_DIR QT_INCLUDE_DIR QT_QT_INCLUDE_DIR QT_DOC_DIR)

  # Set QT_INCLUDES
  SET( QT_INCLUDES ${QT_INCLUDE_DIR} ${QT_QT_INCLUDE_DIR} ${QT_MKSPECS_DIR}/default )

    # Set QT_QTCORE_LIBRARY by searching for a lib with "QtCore."  as part of the filename
    FIND_LIBRARY(QT_QTCORE_LIBRARY_RELEASE NAMES QtCore QtCore4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH )
  FIND_LIBRARY(QT_QTCORE_LIBRARY_DEBUG NAMES QtCore_debug QtCored QtCored4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QT3SUPPORT_LIBRARY
    FIND_LIBRARY(QT_QT3SUPPORT_LIBRARY_RELEASE NAMES Qt3Support Qt3Support4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QT3SUPPORT_LIBRARY_DEBUG   NAMES Qt3Support_debug Qt3Supportd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTGUI_LIBRARY
    FIND_LIBRARY(QT_QTGUI_LIBRARY_RELEASE NAMES QtGui QtGui4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTGUI_LIBRARY_DEBUG   NAMES QtGui_debug QtGuid4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTMOTIF_LIBRARY
    IF(Q_WS_X11)
      FIND_LIBRARY(QT_QTMOTIF_LIBRARY_RELEASE NAMES QtMotif PATHS ${QT_LIBRARY_DIR}       NO_DEFAULT_PATH)
      FIND_LIBRARY(QT_QTMOTIF_LIBRARY_DEBUG   NAMES QtMotif_debug PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)
    ENDIF(Q_WS_X11)

    # Set QT_QTNETWORK_LIBRARY
    FIND_LIBRARY(QT_QTNETWORK_LIBRARY_RELEASE NAMES QtNetwork QtNetwork4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTNETWORK_LIBRARY_DEBUG   NAMES QtNetwork_debug QtNetworkd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTNSPLUGIN_LIBRARY
    FIND_LIBRARY(QT_QTNSPLUGIN_LIBRARY_RELEASE NAMES QtNsPlugin PATHS ${QT_LIBRARY_DIR}       NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTNSPLUGIN_LIBRARY_DEBUG   NAMES QtNsPlugin_debug PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTOPENGL_LIBRARY
    FIND_LIBRARY(QT_QTOPENGL_LIBRARY_RELEASE NAMES QtOpenGL QtOpenGL4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTOPENGL_LIBRARY_DEBUG   NAMES QtOpenGL_debug QtOpenGLd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTSQL_LIBRARY
    FIND_LIBRARY(QT_QTSQL_LIBRARY_RELEASE NAMES QtSql QtSql4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTSQL_LIBRARY_DEBUG   NAMES QtSql_debug QtSqld4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTXML_LIBRARY
    FIND_LIBRARY(QT_QTXML_LIBRARY_RELEASE NAMES QtXml QtXml4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTXML_LIBRARY_DEBUG   NAMES QtXml_debug QtXmld4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)
	
	# Set QT_QTXMLPATTERNS_LIBRARY
    FIND_LIBRARY(QT_QTXMLPATTERNS_LIBRARY_RELEASE NAMES QtXmlPatterns QtXmlPatterns4 PATHS ${QT_LIBRARY_DIR}         NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTXMLPATTERNS_LIBRARY_DEBUG   NAMES QtXmlPatterns_debug QtXmlPatternsd4 PATHS ${QT_LIBRARY_DIR}  NO_DEFAULT_PATH)
	
	# Set QT_QTNETWORK_LIBRARY
    FIND_LIBRARY(QT_QTNETWORK_LIBRARY_RELEASE NAMES QtNetwork QtNetwork4 PATHS ${QT_LIBRARY_DIR}         NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTNETWORK_LIBRARY_DEBUG   NAMES QtNetwork_debug QtNetworkd4 PATHS ${QT_LIBRARY_DIR}  NO_DEFAULT_PATH)

    # Set QT_QTSVG_LIBRARY
    FIND_LIBRARY(QT_QTSVG_LIBRARY_RELEASE NAMES QtSvg QtSvg4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTSVG_LIBRARY_DEBUG   NAMES QtSvg_debug QtSvgd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTUITOOLS_LIBRARY
    FIND_LIBRARY(QT_QTUITOOLS_LIBRARY_RELEASE NAMES QtUiTools QtUiTools4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTUITOOLS_LIBRARY_DEBUG   NAMES QtUiTools_debug QtUiToolsd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    # Set QT_QTTEST_LIBRARY
    FIND_LIBRARY(QT_QTTEST_LIBRARY_RELEASE NAMES QtTest QtTest4 PATHS ${QT_LIBRARY_DIR}                      NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTTEST_LIBRARY_DEBUG   NAMES QtTest_debug QtTest_debug4 QtTestd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

    MARK_AS_ADVANCED(QT_QT3SUPPORT_LIBRARY QT_QTGUI_LIBRARY )

  IF( NOT QT_QTCORE_LIBRARY_DEBUG AND NOT QT_QTCORE_LIBRARY_RELEASE )
    IF( NOT Qt4_FIND_QUIETLY AND Qt4_FIND_REQUIRED)
      MESSAGE( FATAL_ERROR "Could NOT find QtCore. Check ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log for more details.")
    ENDIF( NOT Qt4_FIND_QUIETLY AND Qt4_FIND_REQUIRED)
  ENDIF( NOT QT_QTCORE_LIBRARY_DEBUG AND NOT QT_QTCORE_LIBRARY_RELEASE )

  # Set QT_QTASSISTANT_LIBRARY
  FIND_LIBRARY(QT_QTASSISTANT_LIBRARY_RELEASE NAMES QtAssistantClient QtAssistantClient4 QtAssistant QtAssistant4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)
  FIND_LIBRARY(QT_QTASSISTANT_LIBRARY_DEBUG   NAMES QtAssistantClientd QtAssistantClientd4 QtAssistantClient_debug QtAssistant_debug QtAssistantd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

  # Set QT_QTDESIGNER_LIBRARY
  FIND_LIBRARY(QT_QTDESIGNER_LIBRARY_RELEASE NAMES QtDesigner QtDesigner4 PATHS ${QT_LIBRARY_DIR}        NO_DEFAULT_PATH)
  FIND_LIBRARY(QT_QTDESIGNER_LIBRARY_DEBUG   NAMES QtDesigner_debug QtDesignerd4 PATHS ${QT_LIBRARY_DIR} NO_DEFAULT_PATH)

  # Set QT_QTMAIN_LIBRARY
  IF(WIN32)
    FIND_LIBRARY(QT_QTMAIN_LIBRARY_RELEASE NAMES qtmain PATHS ${QT_LIBRARY_DIR}
      NO_DEFAULT_PATH)
    FIND_LIBRARY(QT_QTMAIN_LIBRARY_DEBUG NAMES qtmaind PATHS ${QT_LIBRARY_DIR}
      NO_DEFAULT_PATH)
  ENDIF(WIN32)

  ############################################
  #
  # Check the existence of the libraries.
  #
  ############################################

  MACRO (_QT4_ADJUST_LIB_VARS basename)
    IF (QT_${basename}_INCLUDE_DIR)

      # if only the release version was found, set the debug variable also to the release version
      IF (QT_${basename}_LIBRARY_RELEASE AND NOT QT_${basename}_LIBRARY_DEBUG)
        SET(QT_${basename}_LIBRARY_DEBUG ${QT_${basename}_LIBRARY_RELEASE})
        SET(QT_${basename}_LIBRARY       ${QT_${basename}_LIBRARY_RELEASE})
        SET(QT_${basename}_LIBRARIES     ${QT_${basename}_LIBRARY_RELEASE})
      ENDIF (QT_${basename}_LIBRARY_RELEASE AND NOT QT_${basename}_LIBRARY_DEBUG)

      # if only the debug version was found, set the release variable also to the debug version
      IF (QT_${basename}_LIBRARY_DEBUG AND NOT QT_${basename}_LIBRARY_RELEASE)
        SET(QT_${basename}_LIBRARY_RELEASE ${QT_${basename}_LIBRARY_DEBUG})
        SET(QT_${basename}_LIBRARY         ${QT_${basename}_LIBRARY_DEBUG})
        SET(QT_${basename}_LIBRARIES       ${QT_${basename}_LIBRARY_DEBUG})
      ENDIF (QT_${basename}_LIBRARY_DEBUG AND NOT QT_${basename}_LIBRARY_RELEASE)
      IF (QT_${basename}_LIBRARY_DEBUG AND QT_${basename}_LIBRARY_RELEASE)
        # if the generator supports configuration types then set
        # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
        IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
          SET(QT_${basename}_LIBRARY       optimized ${QT_${basename}_LIBRARY_RELEASE} debug ${QT_${basename}_LIBRARY_DEBUG})
        ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
          # if there are no configuration types and CMAKE_BUILD_TYPE has no value
          # then just use the release libraries
          SET(QT_${basename}_LIBRARY       ${QT_${basename}_LIBRARY_RELEASE} )
        ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
        SET(QT_${basename}_LIBRARIES       optimized ${QT_${basename}_LIBRARY_RELEASE} debug ${QT_${basename}_LIBRARY_DEBUG})
      ENDIF (QT_${basename}_LIBRARY_DEBUG AND QT_${basename}_LIBRARY_RELEASE)

      SET(QT_${basename}_LIBRARY ${QT_${basename}_LIBRARY} CACHE FILEPATH "The Qt ${basename} library")

      IF (QT_${basename}_LIBRARY)
        SET(QT_${basename}_FOUND 1)
      ENDIF (QT_${basename}_LIBRARY)

      #add the include directory to QT_INCLUDES
      SET(QT_INCLUDES ${QT_INCLUDES} "${QT_${basename}_INCLUDE_DIR}")
    ENDIF (QT_${basename}_INCLUDE_DIR )

    # Make variables changeble to the advanced user
    MARK_AS_ADVANCED(QT_${basename}_LIBRARY QT_${basename}_LIBRARY_RELEASE QT_${basename}_LIBRARY_DEBUG QT_${basename}_INCLUDE_DIR)
  ENDMACRO (_QT4_ADJUST_LIB_VARS)

  IF(WIN32)
    # there is no include for qtmain but adjust macro needs it set
    SET(QT_QTMAIN_INCLUDE_DIR 1)
     _QT4_ADJUST_LIB_VARS(QTMAIN)
    SET(QT_QTMAIN_INCLUDE_DIR )
  ENDIF(WIN32)


  _QT4_ADJUST_LIB_VARS(QTCORE)
  _QT4_ADJUST_LIB_VARS(QTGUI)
  _QT4_ADJUST_LIB_VARS(QT3SUPPORT)
  _QT4_ADJUST_LIB_VARS(QTASSISTANT)
  _QT4_ADJUST_LIB_VARS(QTDESIGNER)
  IF(Q_WS_X11)
    _QT4_ADJUST_LIB_VARS(QTMOTIF)
  ENDIF(Q_WS_X11)
  _QT4_ADJUST_LIB_VARS(QTNETWORK)
  _QT4_ADJUST_LIB_VARS(QTNSPLUGIN)
  _QT4_ADJUST_LIB_VARS(QTOPENGL)
  _QT4_ADJUST_LIB_VARS(QTSQL)
  _QT4_ADJUST_LIB_VARS(QTXML)
  _QT4_ADJUST_LIB_VARS(QTXMLPATTERNS)
  _QT4_ADJUST_LIB_VARS(QTNETWORK)
  _QT4_ADJUST_LIB_VARS(QTSVG)
  _QT4_ADJUST_LIB_VARS(QTUITOOLS)
  _QT4_ADJUST_LIB_VARS(QTTEST)



  #######################################
  #
  #       Check the executables of Qt 
  #          ( moc, uic, rcc )
  #
  #######################################


   # find moc and uic using qmake
  QT_QUERY_QMAKE(QT_MOC_EXECUTABLE_INTERNAL "QMAKE_MOC")
  QT_QUERY_QMAKE(QT_UIC_EXECUTABLE_INTERNAL "QMAKE_UIC")

   FILE(TO_CMAKE_PATH 
     "${QT_MOC_EXECUTABLE_INTERNAL}" QT_MOC_EXECUTABLE_INTERNAL)
   FILE(TO_CMAKE_PATH 
     "${QT_UIC_EXECUTABLE_INTERNAL}" QT_UIC_EXECUTABLE_INTERNAL)

   SET(QT_MOC_EXECUTABLE 
     ${QT_MOC_EXECUTABLE_INTERNAL} CACHE FILEPATH "The moc executable")
   SET(QT_UIC_EXECUTABLE 
     ${QT_UIC_EXECUTABLE_INTERNAL} CACHE FILEPATH "The uic executable")

  FIND_PROGRAM(QT_UIC3_EXECUTABLE
    NAMES uic3
    PATHS ${QT_BINARY_DIR}
    NO_DEFAULT_PATH
    )

  FIND_PROGRAM(QT_RCC_EXECUTABLE 
    NAMES rcc
    PATHS ${QT_BINARY_DIR}
    NO_DEFAULT_PATH
    )

  IF (QT_MOC_EXECUTABLE)
     SET(QT_WRAP_CPP "YES")
  ENDIF (QT_MOC_EXECUTABLE)

  IF (QT_UIC_EXECUTABLE)
     SET(QT_WRAP_UI "YES")
  ENDIF (QT_UIC_EXECUTABLE)



  MARK_AS_ADVANCED( QT_UIC_EXECUTABLE QT_UIC3_EXECUTABLE QT_MOC_EXECUTABLE QT_RCC_EXECUTABLE )

  ######################################
  #
  #       Macros for building Qt files
  #
  ######################################

  MACRO (QT4_GET_MOC_INC_DIRS _moc_INC_DIRS)
     SET(${_moc_INC_DIRS})
     GET_DIRECTORY_PROPERTY(_inc_DIRS INCLUDE_DIRECTORIES)

     FOREACH(_current ${_inc_DIRS})
        SET(${_moc_INC_DIRS} ${${_moc_INC_DIRS}} "-I" ${_current})
     ENDFOREACH(_current ${_inc_DIRS})
  ENDMACRO(QT4_GET_MOC_INC_DIRS)


  MACRO (QT4_GENERATE_MOC infile outfile )
  # get include dirs
     QT4_GET_MOC_INC_DIRS(moc_includes)

     GET_FILENAME_COMPONENT(infile ${infile} ABSOLUTE)

     ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
        COMMAND ${QT_MOC_EXECUTABLE}
        ARGS ${moc_includes} -o ${outfile} ${infile}
        DEPENDS ${infile})

     MACRO_ADD_FILE_DEPENDENCIES(${infile} ${outfile})
  ENDMACRO (QT4_GENERATE_MOC)


  # QT4_WRAP_CPP(outfiles inputfile ... )
  # TODO  perhaps add support for -D, -U and other minor options

  MACRO (QT4_WRAP_CPP outfiles )
    # get include dirs
    QT4_GET_MOC_INC_DIRS(moc_includes)

    FOREACH (it ${ARGN})
      GET_FILENAME_COMPONENT(it ${it} ABSOLUTE)
      GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)

      SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/moc_${outfile}.cc)
      ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
        COMMAND ${QT_MOC_EXECUTABLE}
        ARGS ${moc_includes} -o ${outfile} ${it}
        DEPENDS ${it})
      SET(${outfiles} ${${outfiles}} ${outfile})
    ENDFOREACH(it)

  ENDMACRO (QT4_WRAP_CPP)


  # QT4_WRAP_UI(outfiles inputfile ... )

  MACRO (QT4_WRAP_UI outfiles )

    FOREACH (it ${ARGN})
      GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
      GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
      SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/${outfile}.h)
      ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
        COMMAND ${QT_UIC_EXECUTABLE}
        ARGS -o ${outfile} ${infile}
        MAIN_DEPENDENCY ${infile})
      SET(${outfiles} ${${outfiles}} ${outfile})
    ENDFOREACH (it)

  ENDMACRO (QT4_WRAP_UI)


  # QT4_ADD_RESOURCE(outfiles inputfile ... )
  # TODO  perhaps consider adding support for compression and root options to rcc

  MACRO (QT4_ADD_RESOURCES outfiles )

    FOREACH (it ${ARGN})
      GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
      GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
      GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
      SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cc)
      #  parse file for dependencies
      FILE(READ "${infile}" _RC_FILE_CONTENTS)
      STRING(REGEX MATCHALL "<file>[^<]*" _RC_FILES "${_RC_FILE_CONTENTS}")
      SET(_RC_DEPENDS)
      FOREACH(_RC_FILE ${_RC_FILES})
        STRING(REGEX REPLACE "^<file>" "" _RC_FILE "${_RC_FILE}")
        SET(_RC_DEPENDS ${_RC_DEPENDS} "${rc_path}/${_RC_FILE}")
      ENDFOREACH(_RC_FILE)
      ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
        COMMAND ${QT_RCC_EXECUTABLE}
        ARGS -name ${outfilename} -o ${outfile} ${infile}
        MAIN_DEPENDENCY ${infile}
        DEPENDS ${_RC_DEPENDS})
      SET(${outfiles} ${${outfiles}} ${outfile})
    ENDFOREACH (it)

  ENDMACRO (QT4_ADD_RESOURCES)


   MACRO(QT4_AUTOMOC)
      QT4_GET_MOC_INC_DIRS(_moc_INCS)

      SET(_matching_FILES )
      FOREACH (_current_FILE ${ARGN})

         GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
         # if "SKIP_AUTOMOC" is set to true, we will not handle this file here.
         # here. this is required to make bouic work correctly:
         # we need to add generated .cpp files to the sources (to compile them),
         # but we cannot let automoc handle them, as the .cpp files don't exist yet when
         # cmake is run for the very first time on them -> however the .cpp files might
         # exist at a later run. at that time we need to skip them, so that we don't add two
         # different rules for the same moc file
         GET_SOURCE_FILE_PROPERTY(_skip ${_abs_FILE} SKIP_AUTOMOC)

         IF ( NOT _skip AND EXISTS ${_abs_FILE} )

            FILE(READ ${_abs_FILE} _contents)

            GET_FILENAME_COMPONENT(_abs_PATH ${_abs_FILE} PATH)

            STRING(REGEX MATCHALL "#include +[^ ]+\\.moc[\">]" _match "${_contents}")
            IF(_match)
               FOREACH (_current_MOC_INC ${_match})
                  STRING(REGEX MATCH "[^ <\"]+\\.moc" _current_MOC "${_current_MOC_INC}")

                  GET_filename_component(_basename ${_current_MOC} NAME_WE)
   #               SET(_header ${CMAKE_CURRENT_SOURCE_DIR}/${_basename}.h)
                  SET(_header ${_abs_PATH}/${_basename}.h)
                  SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_current_MOC})
                  ADD_CUSTOM_COMMAND(OUTPUT ${_moc}
                     COMMAND ${QT_MOC_EXECUTABLE}
                     ARGS ${_moc_INCS} ${_header} -o ${_moc}
                     DEPENDS ${_header}
                  )

                  MACRO_ADD_FILE_DEPENDENCIES(${_abs_FILE} ${_moc})
               ENDFOREACH (_current_MOC_INC)
            ENDIF(_match)
         ENDIF ( NOT _skip AND EXISTS ${_abs_FILE} )
      ENDFOREACH (_current_FILE)
   ENDMACRO(QT4_AUTOMOC)



  ######################################
  #
  #       decide if Qt got found
  #
  ######################################

  # if the includes,libraries,moc,uic and rcc are found then we have it
  IF( QT_LIBRARY_DIR AND QT_INCLUDE_DIR AND QT_MOC_EXECUTABLE AND QT_UIC_EXECUTABLE AND QT_RCC_EXECUTABLE)
    SET( QT4_FOUND "YES" )
    IF( NOT Qt4_FIND_QUIETLY)
      MESSAGE(STATUS "Found Qt-Version ${QTVERSION}")
    ENDIF( NOT Qt4_FIND_QUIETLY)
  ELSE( QT_LIBRARY_DIR AND QT_INCLUDE_DIR AND QT_MOC_EXECUTABLE AND QT_UIC_EXECUTABLE AND QT_RCC_EXECUTABLE)
    SET( QT4_FOUND "NO")
    IF( Qt4_FIND_REQUIRED)
      MESSAGE( FATAL_ERROR "Qt libraries, includes, moc, uic or/and rcc NOT found!")
    ENDIF( Qt4_FIND_REQUIRED)
  ENDIF( QT_LIBRARY_DIR AND QT_INCLUDE_DIR AND QT_MOC_EXECUTABLE AND QT_UIC_EXECUTABLE AND  QT_RCC_EXECUTABLE)
  SET(QT_FOUND ${QT4_FOUND})

  #######################################
  #
  #       Qt configuration
  #
  #######################################
  IF(EXISTS "${QT_MKSPECS_DIR}/qconfig.pri")
    FILE(READ ${QT_MKSPECS_DIR}/qconfig.pri _qconfig_FILE_contents)
    STRING(REGEX MATCH "QT_CONFIG[^\n]+" QT_QCONFIG ${_qconfig_FILE_contents})
    STRING(REGEX MATCH "CONFIG[^\n]+" QT_CONFIG ${_qconfig_FILE_contents})
  ENDIF(EXISTS "${QT_MKSPECS_DIR}/qconfig.pri")


  ###############################################
  #
  #       configuration/system dependent settings  
  #
  ###############################################

  SET(QT_GUI_LIB_DEPENDENCIES "")
  SET(QT_CORE_LIB_DEPENDENCIES "")
  
  # shared build needs -DQT_SHARED
  IF(NOT QT_CONFIG MATCHES "static")
    # warning currently only qconfig.pri on Windows potentially contains "static"
    # so QT_SHARED might not get defined properly on Mac/X11 (which seems harmless right now)
    # Trolltech said they'd consider exporting it for all platforms in future releases.
    SET(QT_DEFINITIONS ${QT_DEFINITIONS} -DQT_SHARED)
  ENDIF(NOT QT_CONFIG MATCHES "static")
  
  ## system png
  IF(QT_QCONFIG MATCHES "system-png")
    FIND_LIBRARY(QT_PNG_LIBRARY NAMES png)
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_PNG_LIBRARY})
    MARK_AS_ADVANCED(QT_PNG_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "system-png")
  
  # for X11, get X11 library directory
  IF(Q_WS_X11)
    QT_QUERY_QMAKE(QMAKE_LIBDIR_X11 "QMAKE_LIBDIR_X11")
  ENDIF(Q_WS_X11)

  ## X11 SM
  IF(QT_QCONFIG MATCHES "x11sm")
    # ask qmake where the x11 libs are
    FIND_LIBRARY(QT_X11_SM_LIBRARY NAMES SM PATHS ${QMAKE_LIBDIR_X11})
    FIND_LIBRARY(QT_X11_ICE_LIBRARY NAMES ICE PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_X11_SM_LIBRARY} ${QT_X11_ICE_LIBRARY})
    MARK_AS_ADVANCED(QT_X11_SM_LIBRARY)
    MARK_AS_ADVANCED(QT_X11_ICE_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "x11sm")
  
  ## Xi
  IF(QT_QCONFIG MATCHES "tablet")
    FIND_LIBRARY(QT_XI_LIBRARY NAMES Xi PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_XI_LIBRARY})
    MARK_AS_ADVANCED(QT_XI_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "tablet")

  ## Xrender
  IF(QT_QCONFIG MATCHES "xrender")
    FIND_LIBRARY(QT_XRENDER_LIBRARY NAMES Xrender PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_XRENDER_LIBRARY})
    MARK_AS_ADVANCED(QT_XRENDER_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "xrender")
  
  ## Xrandr
  IF(QT_QCONFIG MATCHES "xrandr")
    FIND_LIBRARY(QT_XRANDR_LIBRARY NAMES Xrandr PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_XRANDR_LIBRARY})
    MARK_AS_ADVANCED(QT_XRANDR_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "xrandr")
  
  ## Xcursor
  IF(QT_QCONFIG MATCHES "xcursor")
    FIND_LIBRARY(QT_XCURSOR_LIBRARY NAMES Xcursor PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_XCURSOR_LIBRARY})
    MARK_AS_ADVANCED(QT_XCURSOR_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "xcursor")
  
  ## Xinerama
  IF(QT_QCONFIG MATCHES "xinerama")
    FIND_LIBRARY(QT_XINERAMA_LIBRARY NAMES Xinerama PATHS ${QMAKE_LIBDIR_X11})
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_XINERAMA_LIBRARY})
    MARK_AS_ADVANCED(QT_XINERAMA_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "xinerama")
  
  ## system-freetype
  IF(QT_QCONFIG MATCHES "system-freetype")
    FIND_LIBRARY(QT_FREETYPE_LIBRARY NAMES freetype)
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_FREETYPE_LIBRARY})
    MARK_AS_ADVANCED(QT_FREETYPE_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "system-freetype")
  
  ## fontconfig
  IF(QT_QCONFIG MATCHES "fontconfig")
    FIND_LIBRARY(QT_FONTCONFIG_LIBRARY NAMES fontconfig)
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${QT_FONTCONFIG_LIBRARY})
    MARK_AS_ADVANCED(QT_FONTCONFIG_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "fontconfig")
  
  ## system-zlib
  IF(QT_QCONFIG MATCHES "system-zlib")
    FIND_LIBRARY(QT_ZLIB_LIBRARY NAMES z)
    SET(QT_CORE_LIB_DEPENDENCIES ${QT_CORE_LIB_DEPENDENCIES} ${QT_ZLIB_LIBRARY})
    MARK_AS_ADVANCED(QT_ZLIB_LIBRARY)
  ENDIF(QT_QCONFIG MATCHES "system-zlib")

  IF(Q_WS_X11)
    # X11 libraries Qt absolutely depends on
    QT_QUERY_QMAKE(QT_LIBS_X11 "QMAKE_LIBS_X11")
    SEPARATE_ARGUMENTS(QT_LIBS_X11)
    FOREACH(QT_X11_LIB ${QT_LIBS_X11})
      STRING(REGEX REPLACE "-l" "" QT_X11_LIB "${QT_X11_LIB}")
      SET(QT_TMP_STR "QT_X11_${QT_X11_LIB}_LIBRARY")
      FIND_LIBRARY(${QT_TMP_STR} NAMES "${QT_X11_LIB}" PATHS ${QMAKE_LIBDIR_X11})
      SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} ${${QT_TMP_STR}})
    ENDFOREACH(QT_X11_LIB)

    QT_QUERY_QMAKE(QT_LIBS_THREAD "QMAKE_LIBS_THREAD")
    SET(QT_CORE_LIB_DEPENDENCIES ${QT_CORE_LIB_DEPENDENCIES} ${QT_LIBS_THREAD})
    
    QT_QUERY_QMAKE(QMAKE_LIBS_DYNLOAD "QMAKE_LIBS_DYNLOAD")
    SET (QT_CORE_LIB_DEPENDENCIES ${QT_CORE_LIB_DEPENDENCIES} ${QMAKE_LIBS_DYNLOAD})

  ENDIF(Q_WS_X11)

  IF(Q_WS_WIN)
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} Imm32 Winmm)
    SET(QT_CORE_LIB_DEPENDENCIES ${QT_CORE_LIB_DEPENDENCIES} Ws2_32)
  ENDIF(Q_WS_WIN)
  
  IF(Q_WS_MAC)
    SET(QT_GUI_LIB_DEPENDENCIES ${QT_GUI_LIB_DEPENDENCIES} "-framework Carbon" "-framework QuickTime")
    SET(QT_CORE_LIB_DEPENDENCIES ${QT_CORE_LIB_DEPENDENCIES} "-framework ApplicationServices")
  ENDIF(Q_WS_MAC)

  #######################################
  #
  #       compatibility settings 
  #
  #######################################
  # Backwards compatibility for CMake1.4 and 1.2
  SET (QT_MOC_EXE ${QT_MOC_EXECUTABLE} )
  SET (QT_UIC_EXE ${QT_UIC_EXECUTABLE} )

  SET( QT_QT_LIBRARY "")

ELSE(QT4_QMAKE_FOUND)

   IF(Qt4_FIND_REQUIRED)
      IF(QT4_INSTALLED_VERSION_TOO_OLD)
         MESSAGE(FATAL_ERROR "The installed Qt version ${QTVERSION} is too old, at least version ${QT_MIN_VERSION} is required")
      ELSE(QT4_INSTALLED_VERSION_TOO_OLD)
     MESSAGE( FATAL_ERROR "Qt qmake not found!")
      ENDIF(QT4_INSTALLED_VERSION_TOO_OLD)
   ELSE(Qt4_FIND_REQUIRED)
      IF(QT4_INSTALLED_VERSION_TOO_OLD AND NOT Qt4_FIND_QUIETLY)
         MESSAGE(STATUS "The installed Qt version ${QTVERSION} is too old, at least version ${QT_MIN_VERSION} is required")
      ENDIF(QT4_INSTALLED_VERSION_TOO_OLD AND NOT Qt4_FIND_QUIETLY)
   ENDIF(Qt4_FIND_REQUIRED)
 
ENDIF (QT4_QMAKE_FOUND)

