/* $Id: Parse.h 10114 2010-11-04 05:44:05Z elau $ */

/**
 * \file
 * Wrapper around <boost/cstdint.hpp> to fix compile errors on Visual Studio 2010.
 *
 * Most recent change:
 *   $Date: 2010-11-04 16:44:05 +1100 (Thu, 04 Nov 2010) $
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

#ifndef GPLATES_SYSTEMFIXES_BOOST_CSTDINT_HPP
#define GPLATES_SYSTEMFIXES_BOOST_CSTDINT_HPP

#include "global/config.h"

#if _MSC_VER >= 1600 // Visual Studio 2010
#	undef UINT8_C
#endif

#include BOOST_CSTDINT_HPP_PATH

#if _MSC_VER >= 1600 // Visual Studio 2010
#	undef UINT8_C
#	include <cstdint>
#endif

#endif  // GPLATES_SYSTEMFIXES_BOOST_CSTDINT_HPP