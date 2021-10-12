/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_GLOBAL_COMPILERWARNINGS_H
#define GPLATES_GLOBAL_COMPILERWARNINGS_H


//
// NOTE: In the macros below we cannot use "#pragma" because the compiler
// interprets the '#' to be the stringizing operator.
// Instead we use the compiler-specific preprocessor operators:
// gcc  - _Pragma()   // Takes a literal string.
// msvc - __pragma()  // Takes tokens that are not in a string.


//
// NOTE: Unfortunately push/pop is only supported by the latest 'gcc' compilers.
// You'll have to re-enable a warning instead of popping the warning state.
//
#if 0
// Push 'gcc' warnings only.
#if defined(__GNUC__)
#define PUSH_GCC_WARNINGS \
		_Pragma( STRINGIFY_WARNING(GCC diagnostic push) )
#else
#define PUSH_GCC_WARNINGS
#endif

// Pop 'gcc' warnings only.
#if defined(__GNUC__)
#	define POP_GCC_WARNINGS \
		_Pragma( STRINGIFY_WARNING(GCC diagnostic pop) )
#else
#	define POP_GCC_WARNINGS
#endif
#endif


/**
 * Stores the current warning state for every warning.
 *
 * Specific to Microsoft Visual Studio.
 */
#if defined(_MSC_VER)
#	define PUSH_MSVC_WARNINGS \
		__pragma( warning( push ) )
#else
#	define PUSH_MSVC_WARNINGS
#endif

/**
 * Pops the last warning state pushed onto the stack by 'PUSH_WARNINGS'.
 *
 * Specific to Microsoft Visual Studio.
 */
#if defined(_MSC_VER)
#	define POP_MSVC_WARNINGS \
		__pragma( warning( pop ) )
#else
#	define POP_MSVC_WARNINGS
#endif


/**
 * Enable a warning in gcc using a warning string.
 *
 * For example:
 *    ENABLE_GCC_WARNING("-Wstrict-overflow")
 */
#if defined(__GNUC__)
#	define ENABLE_GCC_WARNING(warning_string) \
		_Pragma( STRINGIFY_WARNING(GCC diagnostic error warning_string ) )
#else
#	define ENABLE_GCC_WARNING(warning_string)
#endif

/**
 * Disable a warning in gcc using a warning string.
 *
 * For example:
 *    DISABLE_GCC_WARNING("-Wstrict-overflow")
 */
#if defined(__GNUC__)
#	define DISABLE_GCC_WARNING(warning_string) \
		_Pragma( STRINGIFY_WARNING(GCC diagnostic ignored warning_string ) )
#else
#	define DISABLE_GCC_WARNING(warning_string)
#endif


/**
 * Enable a warning in Microsoft Visual Studio using a warning number.
 *
 * For example:
 *    ENABLE_MSVC_WARNING(4181)
 */
#if defined(_MSC_VER)
#	define ENABLE_MSVC_WARNING(warning_number) \
		__pragma( warning( error : warning_number ) )
#else
#	define ENABLE_MSVC_WARNING(warning_number)
#endif

/**
 * Disable a warning in Microsoft Visual Studio using a warning number.
 *
 * For example:
 *    DISABLE_MSVC_WARNING(4181)
 */
#if defined(_MSC_VER)
#	define DISABLE_MSVC_WARNING(warning_number) \
		__pragma( warning( disable : warning_number ) )
#else
#	define DISABLE_MSVC_WARNING(warning_number)
#endif


// Used to support gcc's _Pragma() preprocessor operator which expects a string literal.
#define STRINGIFY_WARNING(warning) #warning

#endif // GPLATES_GLOBAL_COMPILERWARNINGS_H
