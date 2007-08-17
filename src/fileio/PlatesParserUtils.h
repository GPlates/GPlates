/* $Id$ */

/**
 * \file 
 * This file contains a collection of data types which will be used by
 * the PLATES-format parser, and will probably _not_ be used by any other
 * parser.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef _GPLATES_FILEIO_PLATESPARSERUTILS_H_
#define _GPLATES_FILEIO_PLATESPARSERUTILS_H_

#include <sstream>
#include <string>

#include "global/types.h"
#include "PlatesDataTypes.h"
#include "LineBuffer.h"


namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		using namespace GPlatesGlobal;

		int 
		attemptToReadInt(
				const LineBuffer &lb,
				std::istringstream &iss, const char *desc);

		fpdata_t 
		attemptToReadFloat(
				const LineBuffer &lb,
				std::istringstream &iss, const char *desc);

		std::string 
		attemptToReadString(
				const LineBuffer &lb,
				std::istringstream &iss, const char *desc);

		plate_id_t
		attemptToReadPlateID(
				const LineBuffer &lb,
				std::istringstream &iss, const char *desc);

		PlotterCodes::PlotterCode 
		attemptToReadPlotterCode(
				const LineBuffer &lb,
				std::istringstream &iss);
	}
}

#endif  // _GPLATES_FILEIO_PLATESPARSERUTILS_H_
