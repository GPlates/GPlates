/* $Id$ */

/**
 * \file 
 * This file contains a collection of data types which will be used by
 * the PLATES-format parser, and will probably _not_ be used by any other
 * parser.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
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

		int attemptToReadInt(const LineBuffer &lb,
			std::istringstream &iss, const char *desc);

		fpdata_t attemptToReadFloat(const LineBuffer &lb,
			std::istringstream &iss, const char *desc);

		std::string attemptToReadString(const LineBuffer &lb,
			std::istringstream &iss, const char *desc);

		plate_id_t attemptToReadPlateID(const LineBuffer &lb,
			std::istringstream &iss, const char *desc);

		int attemptToReadPlotterCode(const LineBuffer &lb,
			std::istringstream &iss);
	}
}

#endif  // _GPLATES_FILEIO_PLATESPARSERUTILS_H_
