/* $Id$ */

//#define DEBUG_INSERTIONS

/**
 * @file 
 * Contains the NetCDF reader.
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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */


#include <iomanip>
#include <netcdfcpp.h>
#include <sstream>
#include <wx/progdlg.h>
#include "FileFormatException.h"
#include "NetCDFReader.h"
#include "geo/GeologicalData.h"
#include "geo/GridData.h"
#include "geo/GridElement.h"
#include "global/Exception.h"
#include "global/types.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"

using GPlatesGlobal::index_t;
using GPlatesFileIO::FileFormatException;


static GPlatesMaths::PointOnSphere pos (double lat, double lon)
{
	return GPlatesMaths::OperationsOnSphere::
		convertLatLonPointToPointOnSphere
					(GPlatesMaths::LatLonPoint (lat, lon));
}

#ifdef DEBUG_INSERTIONS
static void pos (GPlatesMaths::PointOnSphere pos, double &lat, double &lon)
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::OperationsOnSphere::
					convertPointOnSphereToLatLonPoint (pos);
	lat = llp.latitude ().dval ();
	lon = llp.longitude ().dval ();
}
#endif

GPlatesGeo::GridData *GPlatesFileIO::NetCDFReader::Read (NcFile *ncf,
							wxProgressDialog *dlg)
{
#if 1
	const char *vars[] = { "x_range", "y_range", "z_range",
				"spacing", "dimension", "z" };
	const char *types[] = { "**", "ncByte", "ncChar", "ncShort",
				"ncInt", "ncFloat", "ncDouble" };
	for (int i = 0; i < ncf->num_dims (); ++i) {
		NcDim *dim = ncf->get_dim (i);
		std::cerr << dim->name () << ": a dimension of ";
		if (dim->is_unlimited ())
			std::cerr << "infinite size";
		else
			std::cerr << "size " << dim->size ();
		std::cerr << ".\n";
	}
	for (unsigned int i = 0; i < sizeof (vars) / sizeof (vars[0]); ++i) {
		NcVar *var = ncf->get_var (vars[i]);
		std::cerr << vars[i] << ": a " << var->num_dims ()
			<< "-D " << types[var->type ()]
			<< " variable with " << var->num_atts ()
			<< " attributes and " << var->num_vals ()
			<< " values.\n";
		std::cerr << "\tAttributes: ";
		for (int j = 0; j < var->num_atts (); ++j) {
			NcAtt *att = var->get_att (j);
			std::cerr << att->name () << " ("
				<< types[att->type ()] << ") = ";
			switch (att->type ()) {
				char *str;
				case ncChar:
					str = att->as_string (0);
					std::cerr << '"' << str << '"';
					delete[] str;
					break;
				case ncInt:
					std::cerr << att->as_int (0);
					break;
				case ncFloat:
					std::cerr << att->as_float (0);
					break;
				case ncDouble:
					std::cerr << att->as_double (0);
					break;
				default:
					std::cerr << "?!?";
			}
			std::cerr << ";";
			delete att;
		}
		std::cerr << "\n";
		std::cerr << "\tDimensions: ";
		for (int j = 0; j < var->num_dims (); ++j) {
			NcDim *dim = var->get_dim (j);
			std::cerr << dim->name () << ", ";
		}
		std::cerr << "\b\b \n";
		std::cerr << "\tValues: ";
		if (var->num_vals () < 10) {
			NcValues *vals = var->values ();
			vals->print (std::cerr);
			delete vals;
		} else {
			std::cerr << "(too many - " << var->num_vals () << ")";
		}
		std::cerr << "\n";
	}
#endif

	if (dlg)
		dlg->Update (0, "Checking file...");

	double x_min, x_max, x_step, y_min, y_max, y_step;
	index_t num_x, num_y;
	NcValues *vals;
	int checks;

	// Check for necessary variables, and their types
	checks = 0;
	static const int decimal_mask = (1 << ncShort) | (1 << ncInt) |
					(1 << ncFloat) | (1 << ncDouble);
	static struct {
		const char *name;
		int types;	// bitmask of valid types (e.g. (1 << ncChar))
		int min_values;	// minimum number of values required
	} needed_vars[] = {
		{ "x_range", decimal_mask, 2 },
		{ "y_range", decimal_mask, 2 },
		{ "z_range", decimal_mask, 0 },	// only get units from z_range
		{ "spacing", decimal_mask, 2 },
		{ "z", (1 << ncFloat), 1 }
	};
	size_t num_needed_vars = sizeof (needed_vars) / sizeof (needed_vars[0]);
	for (int i = 0; i < ncf->num_vars (); ++i) {
		NcVar *var = ncf->get_var (i);
		for (size_t j = 0; j < num_needed_vars; ++j) {
			int mask = (1 << j);
			if (checks & mask)
				continue;
			if (!strcmp (var->name (), needed_vars[j].name)) {
				checks |= mask;
				break;
			}
		}
	}
	for (size_t j = 0; j < num_needed_vars; ++j) {
		if (!(checks & (1 << j))) {
			// Missing variable!
			std::ostringstream oss;
			oss << "netCDF file is missing the '"
				<< needed_vars[j].name << "' variable!";
			throw FileFormatException (oss.str ().c_str ());
		}
		NcVar *var = ncf->get_var (needed_vars[j].name);
		if (!(needed_vars[j].types & (1 << var->type ()))) {
			// Bad type
			std::ostringstream oss;
			oss << "'" << needed_vars[j].name << "' variable has "
								"wrong type!";
			throw FileFormatException (oss.str ().c_str ());
		}
		if (var->num_vals () < needed_vars[j].min_values) {
			// Not enough values
			std::ostringstream oss;
			oss << "'" << needed_vars[j].name << "' variable has "
				"too few values (" << var->num_vals ()
				<< "<" << needed_vars[j].min_values << ").";
			throw FileFormatException (oss.str ().c_str ());
		}
	}

	if (dlg)
		dlg->Update (0, "Loading grid lattice...");

	vals = ncf->get_var ("x_range")->values ();
	x_min = vals->as_double (0);
	x_max = vals->as_double (1);
	delete vals;
	vals = ncf->get_var ("y_range")->values ();
	y_min = vals->as_double (0);
	y_max = vals->as_double (1);
	delete vals;
	vals = ncf->get_var ("spacing")->values ();
	x_step = vals->as_double (0);
	y_step = vals->as_double (1);
	delete vals;

	if (x_min > x_max)
		x_step = -x_step;
	if (y_min > y_max)
		y_step = -y_step;

	num_x = index_t ((x_max - x_min) / x_step + 1);
	num_y = index_t ((y_max - y_min) / y_step + 1);

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

	// TODO: handle the case where x is actually longitude, etc.

	// TODO: verify that this attribute actually exists, before reading it!
	NcAtt *z_unit_att = ncf->get_var ("z_range")->get_att ("units");
	char *str = z_unit_att->as_string (0);
	std::string z_units (str);
	delete[] str;
	delete z_unit_att;
	GPlatesGeo::GridData *gdata;
	try {
		double real_x_min = (x_min <= -180.0) ? (x_min + 180.0) : x_min;
		GPlatesMaths::PointOnSphere orig = pos (y_min, real_x_min),
				sc_step = pos (y_min, real_x_min + x_step),
				gc_step = pos (y_min + y_step, real_x_min);
		if ((orig == GPlatesMaths::NorthPole) ||
		    (orig == GPlatesMaths::SouthPole)) {
			throw FileFormatException
				("Can't handle grids with polar origins.");
		}
		gdata = new GPlatesGeo::GridData (
			z_units, GPlatesGeo::GeologicalData::NO_ROTATIONGROUP,
			GPlatesGeo::GeologicalData::NO_TIMEWINDOW,
			GPlatesGeo::GeologicalData::Attributes_t (),
			orig, sc_step, gc_step);
	} catch (FileFormatException &e) {
		throw e;
	} catch (GPlatesGlobal::Exception &e) {
		throw FileFormatException
			("Couldn't determine grid structure from file.");
	}

	NcVar *z_var = ncf->get_var ("z");

	// Check we have enough values
	if (z_var->num_vals () < long (num_x * num_y)) {
		std::ostringstream oss;
		oss << "Data file has too few values ("
			<< ncf->get_var ("z")->num_vals () << " < "
			<< num_x * num_y << " = " << num_x << " * "
			<< num_y << ").";
		throw FileFormatException (oss.str ().c_str ());
	}

	// See if scaling and offset parameters are given
	float z_scale = 1.0, z_offset = 0.0;
	NcAtt *scale_attr = z_var->get_att ("scale_factor");
	if (scale_attr) {
		z_scale = scale_attr->as_float (0);
		delete scale_attr;
	}
	NcAtt *offset_attr = z_var->get_att ("add_offset");
	if (offset_attr) {
		z_offset = offset_attr->as_float (0);
		delete offset_attr;
	}

	// FIXME: I'm taking a guess, and hoping that this all happens in
	//	x-major (i.e. along the y-direction first) order...
	float *z = new float[num_y];
	index_t cnt = 0;
	bool cancelled = false;
	if (dlg)
		dlg->Update (0, "Loading grid...");
	for (index_t i = 0; i < num_x; ++i) {
		if (dlg) {
			double perc = 100 * double (i) / double (num_x);
			int val = int (floor (perc));
			std::ostringstream oss;
			oss << "Loading grid (" << std::setprecision (0)
				<< std::fixed << perc << "%)...";
			wxString w_str (oss.str ().c_str ());
			if (dlg->Update (val, w_str) == FALSE) {
				cancelled = true;
				break;
			}
		}
		z_var->set_cur (i * num_y);
		z_var->get (z, num_y);		// assumes it is ncFloat data
		for (index_t j = 0; j < num_y; ++j, ++cnt) {
			if (isnan (z[j]))
				continue;

			// Apply scaling and offsets
			// TODO: this is a FANTASTIC place for some funky
			//    vectorised optimisations, using some of the
			//    MMX2/SSE/3DNow/AltiVec instructions such as
			//    'pmaddwd' (MMX)
			z[j] = z[j] * z_scale + z_offset;

			// Hack to get around stupid netCDF bug
			index_t real_i = cnt % num_x,
				real_j = num_y - (cnt / num_x) - 1;
			//std::cerr << cnt << "=>" << real_j << "\n";

			GPlatesGeo::GridElement *elt =
					new GPlatesGeo::GridElement (z[j]);
			gdata->Add (elt, real_i, real_j);

#ifdef DEBUG_INSERTIONS
			double lat, lon;
			pos (gdata->resolve (real_i, real_j), lat, lon);
			std::cerr << std::setprecision (2) << std::fixed
				<< "Adding '" << z[j] << "' to (lat="
				<< lat << ", long=" << lon << ").\n";
#endif
		}
	}
	if (dlg) {
		dlg->Update (99, cancelled ? "Cancelled!" : "Done.");
	}

	delete[] z;

	if (cancelled) {
		delete gdata;
		return 0;
	}

	return gdata;
}
