/* $Id$ */

/**
 * @file 
 * Contains the NetCDF writer.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2004 The GPlates Consortium
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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */


#include <iomanip>
#include <netcdfcpp.h>
#include <sstream>
#include <wx/progdlg.h>
#include "NetCDFWriter.h"
#include "geo/GeologicalData.h"
#include "geo/GridData.h"
#include "geo/GridElement.h"
#include "global/Exception.h"
#include "global/types.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"

using GPlatesGlobal::index_t;
using GPlatesFileIO::FileFormatException;


bool GPlatesFileIO::NetCDFReader::Write (const std::string &filename,
			GPlatesGeo::GridData *grid, wxProgressDialog *dlg)
{
	///////////////////////////////////////////////////////////
	// BIG NOTE: The actual ordering of data in the grid is
	//	starting from the top-left, working to the right,
	//	then down a row, etc. For example,
	//			1  2  3  4
	//			5  6  7  8
	//			9 10 11 12
	//	would go in numerical order, with latitude increasing
	//	upwards, and longitude increasing to the right.
	///////////////////////////////////////////////////////////

	// TODO: Can't actually do this until GeologicalData is implemented
}
