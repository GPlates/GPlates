# Copied from CMakeFindFrameworks.cmake in CMake 2.6.2.

# - helper module to find OSX frameworks

IF(NOT GPLATES_FIND_FRAMEWORKS_INCLUDED)
  SET(GPLATES_FIND_FRAMEWORKS_INCLUDED 1)
  MACRO(GPLATES_FIND_FRAMEWORKS fwk)
    SET(${fwk}_FRAMEWORKS)
    IF(APPLE)
      FOREACH(dir
          ~/Library/Frameworks/${fwk}.framework
          ${CMAKE_FRAMEWORK_PATH}/${fwk}.framework # GPlates addition.
          /Library/Frameworks/${fwk}.framework
          /System/Library/Frameworks/${fwk}.framework
          /Network/Library/Frameworks/${fwk}.framework
          /opt/local/Library/Frameworks/${fwk}.framework)
        IF(EXISTS ${dir})
          SET(${fwk}_FRAMEWORKS ${${fwk}_FRAMEWORKS} ${dir})
        ENDIF(EXISTS ${dir})
      ENDFOREACH(dir)
    ENDIF(APPLE)
  ENDMACRO(GPLATES_FIND_FRAMEWORKS)
ENDIF(NOT GPLATES_FIND_FRAMEWORKS_INCLUDED)
