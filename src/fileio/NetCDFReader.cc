/* $Id$ */

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

GPlatesGeo::GridData *GPlatesFileIO::NetCDFReader::Read (NcFile *ncf,
							wxProgressDialog *dlg)
{
#if 0
	const char *vars[] = { "x_range", "y_range", "z_range",
				"spacing", "dimension", "z" };
	const char *types[] = { "**", "ncByte", "ncChar", "ncShort",
				"ncInt", "ncFloat", "ncDouble" };
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
					delete str;
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
		{ "z", decimal_mask, 1 }
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

	// TODO: handle the case where x is actually longitude, etc.

	// TODO: verify that this attribute actually exists, before reading it!
	NcAtt *z_unit_att = ncf->get_var ("z_range")->get_att ("units");
	char *str = z_unit_att->as_string (0);
	std::string z_units (str);
	delete[] str;
	delete z_unit_att;
	GPlatesGeo::GridData *gdata;
	try {
		GPlatesMaths::PointOnSphere orig = pos (x_min, y_min),
				sc_step = pos (x_min + x_step, y_min),
				gc_step = pos (x_min, y_min + y_step);
		gdata = new GPlatesGeo::GridData (
			z_units, GPlatesGeo::GeologicalData::NO_ROTATIONGROUP,
			GPlatesGeo::GeologicalData::NO_TIMEWINDOW,
			GPlatesGeo::GeologicalData::Attributes_t (),
			orig, sc_step, gc_step);
	} catch (GPlatesGlobal::Exception &e) {
		throw FileFormatException("Couldn't determine grid from file.");
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
			if (dlg->Update (val, "Loading grid...") == FALSE) {
				cancelled = true;
				break;
			}
		}
		z_var->set_cur (i * num_y);
		if (z_var->get (z, num_y) == FALSE) {
			std::cerr << "ARGH!\n";
			break;
		}
		for (index_t j = 0; j < num_y; ++j, ++cnt) {
			if (isnan (z[j]))
				continue;

			GPlatesGeo::GeologicalData::Attributes_t attr =
				GPlatesGeo::GeologicalData::NO_ATTRIBUTES;
			// TODO: insert properly when StringValue is
			//	actually useful (value is in z[j])
			GPlatesGeo::GridElement *elt =
					new GPlatesGeo::GridElement (attr);
			gdata->Add (elt, i, j);
		}
	}
	delete[] z;

	if (cancelled) {
		delete gdata;
		return 0;
	}

	return gdata;
}
