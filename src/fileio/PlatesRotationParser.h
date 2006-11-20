/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Date$
 *
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_FILEIO_PLATESROTATIONPARSER_H_
#define _GPLATES_FILEIO_PLATESROTATIONPARSER_H_

#include <iostream>
#include <string>

#include "global/types.h"
#include "PlatesDataTypes.h"
#include "LineBuffer.h"


namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		using GPlatesGlobal::fpdata_t;

		typedef std::list< RotationSequence > PlatesRotationData;

		void ReadInRotationData(const char *filename,
		 std::istream &input_stream,
		 PlatesRotationData &rotation_data);

		void ReadRotation(LineBuffer &lb,
		 PlatesRotationData &rotation_data);

		void ReadRotationLine(LineBuffer &lb, std::string &str);
	}
}

#endif // _GPLATES_FILEIO_PLATESROTATIONPARSER_H_
