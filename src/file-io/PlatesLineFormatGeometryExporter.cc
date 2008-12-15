/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include "PlatesLineFormatGeometryExporter.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/Real.h"
#include "utils/StringFormattingUtils.h"

namespace
{
	/**
	 * A point on a polyline in the PLATES4 format includes a "draw command"
	 * after the coordninates of the point.  This is a number (2 or 3) which 
	 * tells us whether to draw a line (from the previous point) to the point,
	 * or to start the next line at the point.
	 */
	namespace PenPositions
	{
		enum PenPosition
		{
			PEN_DRAW_TO_POINT = 2, PEN_SKIP_TO_POINT = 3
		};
	}

	
	/**
	 * Adapted from PlatesLineFormatWriter to work on a QTextStream.
	 */
	void
	print_plates_coordinate_line(
			QTextStream &stream,
			const GPlatesMaths::Real &lat,
			const GPlatesMaths::Real &lon,
			PenPositions::PenPosition pen,
			bool reverse_coordinate_order)
	{
		/*
		 * A coordinate in the PLATES4 format is written as decimal number with
		 * 4 digits precision after the decimal point, and it must take up 9
		 * characters altogether (i.e. including the decimal point and maybe
		 * a sign).
		 */
		static const unsigned PLATES_COORDINATE_PRECISION = 4;
		static const unsigned PLATES_COORDINATE_FIELDWIDTH = 9;
		static const unsigned PLATES_PEN_FIELDWIDTH = 1;

		/*
		 * We convert the coordinates to strings first, so that in case an exception
		 * is thrown, the ostream is not modified.
		 */
		std::string lat_str, lon_str, pen_str;
		try {
			lat_str = GPlatesUtils::formatted_double_to_string(lat.dval(),
				PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION);
			lon_str = GPlatesUtils::formatted_double_to_string(lon.dval(),
				PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION);
			pen_str = GPlatesUtils::formatted_int_to_string(static_cast<int>(pen),
				PLATES_PEN_FIELDWIDTH);
		} catch (const GPlatesUtils::InvalidFormattingParametersException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.
			throw;
		}

		if (reverse_coordinate_order) {
			// For whatever perverse reason, the user wants to write in (lon,lat) order.
			stream << lon_str.c_str() << " " << lat_str.c_str() << " " << pen_str.c_str() << endl;
		} else {
			// Normal PLATES4 (lat,lon) order should be used.
			stream << lat_str.c_str() << " " << lon_str.c_str() << " " << pen_str.c_str() << endl;
		}
	}


	void
	print_plates_feature_termination_line(
			QTextStream &stream)
	{
		print_plates_coordinate_line(stream, 99.0, 99.0, PenPositions::PEN_SKIP_TO_POINT, false);
	}


	void
	print_plates_coordinate_line(
			QTextStream &stream,
			const GPlatesMaths::PointOnSphere &pos,
			PenPositions::PenPosition pen,
			bool reverse_coordinate_order)
	{
		GPlatesMaths::LatLonPoint llp =
				GPlatesMaths::make_lat_lon_point(pos);
		print_plates_coordinate_line(stream, llp.latitude(), llp.longitude(), pen,
				reverse_coordinate_order);
	}
}


// FIXME: For now, I'm defining this visitor as part of the GPlatesFileIO
// namespace, and putting it in src/file-io/. It should probably be in src/geometry-visitors/,
// but that requires non-trivial modifications to a build system which will (ideally) be
// deprecated soon in favour of CMake.
GPlatesFileIO::PlatesLineFormatGeometryExporter::PlatesLineFormatGeometryExporter(
		QTextStream &output_stream,
		bool reverse_coordinate_order,
		bool polygon_terminating_point):
	GPlatesMaths::ConstGeometryOnSphereVisitor(),
	d_stream_ptr(&output_stream),
	d_reverse_coordinate_order(reverse_coordinate_order),
	d_polygon_terminating_point(polygon_terminating_point)
{
}


void
GPlatesFileIO::PlatesLineFormatGeometryExporter::export_geometry(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr)
{
#if 0
	qDebug(Q_FUNC_INFO);
#endif
	// Write the coordinate list of the geometry.
	geometry_ptr->accept_visitor(*this);
	// Write the final terminating point.
	print_plates_feature_termination_line(*d_stream_ptr);
}


void
GPlatesFileIO::PlatesLineFormatGeometryExporter::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
#if 0
	qDebug(Q_FUNC_INFO);
#endif
	// Skip-to then draw-to the same location, producing a point.
	print_plates_coordinate_line(*d_stream_ptr, *point_on_sphere, PenPositions::PEN_SKIP_TO_POINT,
			d_reverse_coordinate_order);
	print_plates_coordinate_line(*d_stream_ptr, *point_on_sphere, PenPositions::PEN_DRAW_TO_POINT,
			d_reverse_coordinate_order);
}


void
GPlatesFileIO::PlatesLineFormatGeometryExporter::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
#if 0
	qDebug(Q_FUNC_INFO);
#endif
	// Write out each point of the polygon.
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = polygon_on_sphere->vertex_begin();
	GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon_on_sphere->vertex_end();

	// The first point will need to be a "skip-to" to put the pen in the correct location.
	print_plates_coordinate_line(*d_stream_ptr, *iter, PenPositions::PEN_SKIP_TO_POINT,
			d_reverse_coordinate_order);
	++iter;

	// All subsequent points are "draw-to" to produce the line segments.
	for ( ; iter != end; ++iter) {
		print_plates_coordinate_line(*d_stream_ptr, *iter, PenPositions::PEN_DRAW_TO_POINT,
				d_reverse_coordinate_order);
	}
	
	// Finally, to produce a closed polygon ring with PLATES4 draw commands, we should
	// return to the initial point (Assuming that option was specified, which it is
	// by default)
	if (d_polygon_terminating_point) {
		print_plates_coordinate_line(*d_stream_ptr, *polygon_on_sphere->vertex_begin(),
				PenPositions::PEN_DRAW_TO_POINT, d_reverse_coordinate_order);
	}
}


void
GPlatesFileIO::PlatesLineFormatGeometryExporter::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
#if 0
	qDebug(Q_FUNC_INFO);
#endif
	// Write out each point of the polyline.
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_on_sphere->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_on_sphere->vertex_end();

	// The first point will need to be a "skip-to" to put the pen in the correct location.
	print_plates_coordinate_line(*d_stream_ptr, *iter, PenPositions::PEN_SKIP_TO_POINT,
			d_reverse_coordinate_order);
	++iter;

	// All subsequent points are "draw-to" to produce the line segments.
	for ( ; iter != end; ++iter) {
		print_plates_coordinate_line(*d_stream_ptr, *iter, PenPositions::PEN_DRAW_TO_POINT,
				d_reverse_coordinate_order);
	}
}


