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
#include "FileAccessException.h"
#include "FileFormatException.h"
#include "NetCDFWriter.h"
#include "geo/GeologicalData.h"
#include "geo/GridData.h"
#include "geo/GridElement.h"
#include "global/Exception.h"
#include "global/types.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"

using GPlatesGlobal::index_t;
using GPlatesFileIO::FileAccessException;
using GPlatesFileIO::FileFormatException;


static inline GPlatesMaths::LatLonPoint llp (const GPlatesMaths::PointOnSphere
									&pos)
{
	return GPlatesMaths::OperationsOnSphere::
				convertPointOnSphereToLatLonPoint (pos);
}


bool GPlatesFileIO::NetCDFWriter::Write (const std::string &filename,
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

	NcFile ncf (filename.c_str (), NcFile::Replace);

	if (!ncf.is_valid ()) {
		std::ostringstream oss;
		oss << "Can't create netCDF file \"" << filename << "\".";
		throw new FileAccessException (oss.str ().c_str ());
	}

	//
	// FIXME: This doesn't account for grid rotation in any way!
	//
	index_t nx, ny;
	grid->getDimensions (nx, ny);
	const GPlatesMaths::GridOnSphere &lattice = grid->getLattice ();
	GPlatesMaths::PointOnSphere orig = lattice.resolve (0, 0);
	GPlatesMaths::LatLonPoint orig_llp = llp (orig);
	double lat_step = radiansToDegrees (lattice.deltaAlongLat ()).dval (),
		lon_step = radiansToDegrees (lattice.deltaAlongLon ()).dval ();
	double orig_lat = orig_llp.latitude ().dval (),
		orig_lon = orig_llp.longitude ().dval (),
		corner_lat = orig_lat + lat_step * (ny - 1),
		corner_lon = orig_lon + lon_step * (nx - 1);

	GPlatesMaths::LatLonPoint
		x00 = llp (lattice.resolve (0, 0)),
		x01 = llp (lattice.resolve (0, 1)),
		x10 = llp (lattice.resolve (1, 0)),
		x11 = llp (lattice.resolve (1, 1));
	std::cerr << "First four grid points:\n"
		<< "\tbottom row: " << x00 << ", " << x10 << "\n"
		<< "\t1st row: " << x01 << ", " << x11 << "\n";

	// Create dimensions
	NcDim *dim_side = ncf.add_dim ("side", 2);
	//NcDim *dim_xysize = ncf.add_dim ("xysize", nx * ny);

	// Create variables and their attributes
	NcVar *var_x_range = ncf.add_var ("x_range", ncDouble, dim_side);
	var_x_range->add_att ("units", "deg");
	NcVar *var_y_range = ncf.add_var ("y_range", ncDouble, dim_side);
	var_y_range->add_att ("units", "deg");
	NcVar *var_z_range = ncf.add_var ("z_range", ncDouble, dim_side);
	std::string z_units = grid->GetDataType ();
	var_z_range->add_att ("units", z_units.c_str ());
	NcVar *var_spacing = ncf.add_var ("spacing", ncDouble, dim_side);
	var_spacing->add_att ("units", "deg");
	NcVar *var_dimension = ncf.add_var ("dimension", ncInt, dim_side);
	//NcVar *var_z = ncf.add_var ("z", ncFloat, dim_xysize);
	//var_z->add_att ("scale_factor", (double) 1.0);
	//var_z->add_att ("add_offset", (double) 0.0);

	// Leave define mode and start writing data

	double x_range[2] = { orig_lon, corner_lon };
	var_x_range->put (x_range, 2);
	double y_range[2] = { orig_lat, corner_lat };
	var_y_range->put (y_range, 2);
	double z_range[2] = { grid->min (), grid->max () };
	var_z_range->put (z_range, 2);
	double spacing[2] = { lon_step, lat_step };
	var_spacing->put (spacing, 2);
	int dimension[2] = { nx, ny };
	var_dimension->put (dimension, 2);

	return false;
}
