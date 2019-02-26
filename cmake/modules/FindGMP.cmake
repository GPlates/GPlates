#Use this cmake script to find gmp


FIND_PATH(
	GMP_INCLUDE_DIR 
	gmp.h 
	PATHS
	$GMP_DIR/include
	/opt/local/include
	/usr/local/include
	/opt/include
	DOC "Looking for gmp include path.")

FIND_LIBRARY(
	GMP_LIBRARY 
	NAMES gmp 
	PATH 
	$GMP_DIR/lib
	/opt/local/lib
	/usr/local/lib
	/opt/lib
	DOC "Looking for gmp library file.")

# Fix cmake error finding GMP library on Windows.
# The CGAL internal cmake modules already find GMP and set GMP_FOUND to TRUE, so we need to reset it.
# Finding GMP is not actually required on Windows since recent versions of CGAL use DLLs
# which have already linked to GMP and hence GPlates doesn't need to link to it.
SET(GMP_FOUND FALSE)

IF (GMP_INCLUDE_DIR AND GMP_LIBRARY)
   SET(GMP_FOUND TRUE)
ENDIF (GMP_INCLUDE_DIR AND GMP_LIBRARY)

IF (GMP_FOUND)
   IF (NOT GMP_FIND_QUIETLY)
      MESSAGE(STATUS "Found GMP: ${GMP_LIBRARY}")
   ENDIF (NOT GMP_FIND_QUIETLY)
ELSE (GMP_FOUND)
   IF (GMP_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find GMP")
   ENDIF (GMP_FIND_REQUIRED)
ENDIF (GMP_FOUND)

