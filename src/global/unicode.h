/* $Id: types.h 7999 2010-04-13 04:21:20Z elau $ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date: 2010-04-13 14:21:20 +1000 (Tue, 13 Apr 2010) $
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

#ifndef GPLATES_GLOBAL_UNICODE_H
#define GPLATES_GLOBAL_UNICODE_H

#include "utils/UnicodeString.h"

using GPlatesUtils::UnicodeString;

#if 0
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/schriter.h>
#if _MSC_VER == 1600 // Visual Studio 2010
#	undef INT8_MIN
#	undef INT16_MIN
#	undef INT32_MIN
#	undef INT8_MAX
#	undef INT16_MAX
#	undef INT32_MAX
#	undef UINT8_MAX
#	undef UINT16_MAX
#	undef UINT32_MAX
#	undef INT64_C
#	undef UINT64_C
#	include <cstdint>
#endif // _MSC_VER == 1600
#endif

#endif  // GPLATES_GLOBAL_UNICODE_H
