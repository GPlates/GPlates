/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Author$
 * $Date$
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
 *   Mike Dowman <mdowman@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_PLATESROTATIONPARSER_H_
#define _GPLATES_FILEIO_PLATESROTATIONPARSER_H_

#include <istream>
#include <string>
#include <map>

#include "global/types.h"
#include "PlatesDataTypes.h"
#include "LineBuffer.h"


namespace GPlatesFileIO
{
	namespace PlatesParser
	{
		using GPlatesGlobal::fpdata_t;

		typedef std::multimap< fpdata_t, FiniteRotation >
		 FiniteRotationsOfPlateMap;

		typedef std::map< plate_id_t, FiniteRotationsOfPlateMap >
		 RotationDataMap;

		void ReadInPlateRotationData(const char *filename,
		 std::istream &input_stream, RotationDataMap &rotation_data);

		void ReadRotation(LineBuffer &lb,
		 RotationDataMap &rotation_data);

		void ReadRotationLine(LineBuffer &lb, std::string &str);
	}
}

#endif // _GPLATES_FILEIO_PLATESROTATIONPARSER_H_
