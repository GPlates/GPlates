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
#include "NetCDFReader.h"
#include "geo/GeologicalData.h"
#include "geo/GridData.h"
#include "geo/TimeWindow.h"
#include "maths/OperationsOnSphere.h"
#include "maths/PointOnSphere.h"


static GPlatesMaths::PointOnSphere pos (double lat, double lon)
{
	return GPlatesMaths::OperationsOnSphere::
		convertLatLonPointToPointOnSphere
					(GPlatesMaths::LatLonPoint (lat, lon));
}

GPlatesGeo::GridData *GPlatesFileIO::NetCDFReader::Read (NcFile *ncf)
{
#if 1
	const char *vars[] = { "x_range", "y_range", "z_range",
				"spacing", "dimension", "z" };
	const char *types[] = { "**", "ncByte", "ncChar", "ncShort",
				"ncInt", "ncFloat", "ncDouble" };
	for (int i = 0; i < sizeof (vars) / sizeof (vars[0]); ++i) {
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
				case ncChar:
					std::cerr << '"'
						<< att->as_string (0)
						<< '"';
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
		NcValues *vals = var->values ();
		if (var->num_vals () < 10)
			vals->print (std::cerr);
		else
			std::cerr << "(too many - " << var->num_vals ()
				<< ")";
		std::cerr << "\n";
		delete vals;
	}
#endif

	// TODO: _much_ more error checking is needed here

	double x_min, x_max, x_step, y_min, y_max, y_step;
	NcValues *vals;

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

	GPlatesMaths::PointOnSphere orig = pos (x_min, y_min),
				sc_step = pos (x_min + x_step, y_min),
				gc_step = pos (x_min, y_min + y_step);

	NcAtt *z_units = ncf->get_var ("z_range")->get_att ("units");
	GPlatesGeo::GridData *gdata = new GPlatesGeo::GridData (
		std::string (z_units->as_string (0)), 0,
		GPlatesGeo::TimeWindow (),
		GPlatesGeo::GeologicalData::Attributes_t (),
		orig, sc_step, gc_step);
	delete z_units;

	return gdata;
}
