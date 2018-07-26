#
# CMake policies give the choice of OLD (backwards compatible) or NEW (non-backwards compatible) behaviour
# when a version of CMake newer than specified in 'cmake_minimum_required()' is used.
# CMake states that ultimately all policies should specify NEW with OLD being a temporary solution.
#
# Each 'cmake_policy' command is surrounded by if/endif since the runtime version of CMake may not support them.
# For example, when specifying CMake 3.x policies but the runtime CMake is 2.8.8.
#
if(COMMAND cmake_policy)
    # NEW = A minimum required CMake version must be specified.
	if (POLICY CMP0000)
		cmake_policy(SET CMP0000 OLD)
	endif(POLICY CMP0000)
    
    # NEW = CMAKE_BACKWARDS_COMPATIBILITY should no longer be used.
	if (POLICY CMP0001)
		cmake_policy(SET CMP0001 OLD)
	endif(POLICY CMP0001)
    
    # NEW = Logical target names must be globally unique.
	if (POLICY CMP0002)
		cmake_policy(SET CMP0002 NEW)
	endif(POLICY CMP0002)
    
    # NEW = Libraries linked via full path no longer produce linker search paths.
	if (POLICY CMP0003)
		cmake_policy(SET CMP0003 NEW)
	endif(POLICY CMP0003)
    
    # NEW = Libraries linked may not have leading or trailing whitespace.
	if (POLICY CMP0004)
		cmake_policy(SET CMP0004 OLD)
	endif(POLICY CMP0004)
    
    # NEW = Preprocessor definition values are now escaped automatically.
	if (POLICY CMP0005)
		cmake_policy(SET CMP0005 OLD)
	endif(POLICY CMP0005)
    
    # NEW = Installing MACOSX_BUNDLE targets requires a BUNDLE DESTINATION.
	if (POLICY CMP0006)
		cmake_policy(SET CMP0006 OLD)
	endif(POLICY CMP0006)
    
    # NEW = list command no longer ignores empty elements.
	if (POLICY CMP0007)
		cmake_policy(SET CMP0007 OLD)
	endif(POLICY CMP0007)
    
    # NEW = Libraries linked by full-path must have a valid library file name.
	if (POLICY CMP0008)
		cmake_policy(SET CMP0008 NEW)
	endif(POLICY CMP0008)
    
    # NEW = FILE GLOB_RECURSE calls should not follow symlinks by default.
	if (POLICY CMP0009)
		cmake_policy(SET CMP0009 NEW)
	endif(POLICY CMP0009)
    
    # NEW = Bad variable reference syntax is an error.
	if (POLICY CMP0010)
		cmake_policy(SET CMP0010 NEW)
	endif(POLICY CMP0010)
    
    # NEW = Included scripts do automatic cmake_policy PUSH and POP.
	if (POLICY CMP0011)
		cmake_policy(SET CMP0011 NEW)
	endif(POLICY CMP0011)
endif(COMMAND cmake_policy)
