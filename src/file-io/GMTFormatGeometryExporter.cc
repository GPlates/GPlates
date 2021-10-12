/* $Id$ */

/**
 * \file 
 * Exports GeometryOnSphere derived objects to GMT xy format file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <QtGlobal>
#include <QTextStream>
#include <string>

#include "GMTFormatGeometryExporter.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/Real.h"
#include "utils/StringFormattingUtils.h"

namespace
{
	/**
	* Adapted from GMTFormatWriter to work on a QTextStream.
	*/
	void
		print_gmt_coordinate_line(
		QTextStream &stream,
		const GPlatesMaths::Real &lat,
		const GPlatesMaths::Real &lon,
		bool reverse_coordinate_order)
	{
		/*
		* A coordinate in the GMT xy format is written as decimal number that
		* takes up 8 characters excluding sign.
		*/
		static const unsigned GMT_COORDINATE_FIELDWIDTH = 9;

		/*
		* We convert the coordinates to strings first, so that in case an exception
		* is thrown, the ostream is not modified.
		*/
		std::string lat_str, lon_str;
		try {
			lat_str = GPlatesUtils::formatted_double_to_string(lat.dval(),
				GMT_COORDINATE_FIELDWIDTH);
			lon_str = GPlatesUtils::formatted_double_to_string(lon.dval(),
				GMT_COORDINATE_FIELDWIDTH);
		} catch (const GPlatesUtils::InvalidFormattingParametersException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.
			throw;
		}

		// GMT format is by default (lon,lat) which is opposite of PLATES4 line format.
		if (reverse_coordinate_order) {
			// For whatever perverse reason, the user wants to write in (lat,lon) order.
			stream << "  " << lat_str.c_str()
				<< "      " << lon_str.c_str() << endl;
		} else {
			// Normal GMT (lon,lat) order should be used.
			stream << "  " << lon_str.c_str()
				<< "      " << lat_str.c_str() << endl;
		}
	}


	void
		print_gmt_feature_termination_line(
		QTextStream &stream)
	{
		// No newline is output since a GMT header may follow in which
		// case it will use the same line.
		stream << ">";
	}


	void
		print_gmt_coordinate_line(
		QTextStream &stream,
		const GPlatesMaths::PointOnSphere &pos,
		bool reverse_coordinate_order)
	{
		GPlatesMaths::LatLonPoint llp =
			GPlatesMaths::make_lat_lon_point(pos);
		print_gmt_coordinate_line(stream, llp.latitude(), llp.longitude(),
			reverse_coordinate_order);
	}
}


// FIXME: For now, I'm defining this visitor as part of the GPlatesFileIO
// namespace, and putting it in src/file-io/. It should probably be in src/geometry-visitors/,
// but that requires non-trivial modifications to a build system which will (ideally) be
// deprecated soon in favour of CMake.
GPlatesFileIO::GMTFormatGeometryExporter::GMTFormatGeometryExporter(
	QTextStream &output_stream,
	bool reverse_coordinate_order,
	bool polygon_terminating_point):
d_stream_ptr(&output_stream),
d_reverse_coordinate_order(reverse_coordinate_order),
d_polygon_terminating_point(polygon_terminating_point)
{
}


void
GPlatesFileIO::GMTFormatGeometryExporter::export_geometry(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
{
	// Write the coordinate list of the geometry.
	geometry_ptr->accept_visitor(*this);
	// Write the final terminating symbol.
	print_gmt_feature_termination_line(*d_stream_ptr);
}


void
GPlatesFileIO::GMTFormatGeometryExporter::visit_point_on_sphere(
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	print_gmt_coordinate_line(*d_stream_ptr, *point_on_sphere, d_reverse_coordinate_order);
}


void
GPlatesFileIO::GMTFormatGeometryExporter::visit_polygon_on_sphere(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// Write out each point of the polygon.
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = polygon_on_sphere->vertex_begin();
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon_on_sphere->vertex_end();

	// Output all the points of the polygon.
	for ( ; iter != end; ++iter) {
		print_gmt_coordinate_line(*d_stream_ptr, *iter, d_reverse_coordinate_order);
	}

	// Finally, to produce a closed polygon ring, we should return to the initial point
	// (Assuming that option was specified, which it is by default).
	if (d_polygon_terminating_point) {
		print_gmt_coordinate_line(*d_stream_ptr, *polygon_on_sphere->vertex_begin(),
			d_reverse_coordinate_order);
	}
}


void
GPlatesFileIO::GMTFormatGeometryExporter::visit_polyline_on_sphere(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	// Write out each point of the polyline.
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_on_sphere->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_on_sphere->vertex_end();

	// Output all points to produce the line segments.
	for ( ; iter != end; ++iter) {
		print_gmt_coordinate_line(*d_stream_ptr, *iter, d_reverse_coordinate_order);
	}
}
