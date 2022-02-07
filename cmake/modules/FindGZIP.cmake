

IF(WIN32)
	SET(GZIP_SEARCH_DIR
		${GZIP_SEARCH_DIR}
		"C:/Program Files/GnuWin32/bin")
	SET(GZIP_SEARCH_DIR
		${GZIP_SEARCH_DIR}
		"$ENV{PATH}")
ELSE(WIN32)
	SET( GZIP_SEARCH_DIR 
		/bin
		/usr/bin
		/usr/local/bin
		/opt/local/bin
		/opt/local/sbin
		/sw/bin)
ENDIF(WIN32)

FIND_PROGRAM(GZIP_TOOL
	gzip
	PATHS ${GZIP_SEARCH_DIR})

