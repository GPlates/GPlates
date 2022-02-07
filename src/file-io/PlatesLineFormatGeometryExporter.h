/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PLATESLINEFORMATGEOMETRYEXPORTER_H
#define GPLATES_FILEIO_PLATESLINEFORMATGEOMETRYEXPORTER_H

#include <QTextStream>
#include <boost/noncopyable.hpp>

#include "GeometryExporter.h"
#include "maths/ConstGeometryOnSphereVisitor.h"

// FIXME: For now, I'm defining this visitor as part of the GPlatesFileIO
// namespace, and putting it in src/file-io/. It should probably be in src/geometry-visitors/,
// but that requires non-trivial modifications to a build system which will (ideally) be
// deprecated soon in favour of CMake.
namespace GPlatesFileIO
{
	/**
	 * This class is a ConstGeometryOnSphereVisitor which will output PLATES4
	 * compatible pen commands for the geometry it visits.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 */
	class PlatesLineFormatGeometryExporter :
			public GPlatesFileIO::GeometryExporter,
			private GPlatesMaths::ConstGeometryOnSphereVisitor,
			private boost::noncopyable
	{
	public:
		PlatesLineFormatGeometryExporter(
				QTextStream &output_stream,
				bool reverse_coordinate_order = false,
				bool polygon_terminating_point = true);
		
		virtual
		~PlatesLineFormatGeometryExporter()
		{  }


		/**
		 * Export a geometry and write the final terminating point.
		 * It is not possible to call ->accept_visitor(*this) on the geometry since
		 * we inherit privately from GPlatesMaths::ConstGeometryOnSphereVisitor.
		 */
		virtual
		void
		export_geometry(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr);

		/**
		 * Export one or more geometries of a feature and write the final
		 * terminating point after the last geometry.
		 * The caller is responsible for assembling the geometry(s) or a feature.
		 * 'GeometryForwardIter' is a forward iterator over GPlatesMaths::GeometryOnSphere
		 * pointers (or anything that acts like a pointer).
		 */
		template <typename GeometryForwardIter>
		void
		export_feature_geometries(
				GeometryForwardIter geometries_begin,
				GeometryForwardIter geometries_end);

	private:
		
		/**
		 * The QTextStream we write to.
		 * QTextStreams can conveniently be created from QIODevices and QByteArrays.
		 */
		QTextStream *d_stream_ptr;
		
		/**
		 * Should we go against the norm and write out coordinates using a (lon,lat) ordering?
		 */
		bool d_reverse_coordinate_order;
		
		/**
		 * Should we convert gml:Polygons to something the PLATES line format can render,
		 * by adding an additional terminating point identical to the first point?
		 */
		bool d_polygon_terminating_point;


		// Please keep these geometries ordered alphabetically.

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

		/**
		 * Writes the terminating point to signal no more geometry(s) for a feature.
		 */
		void
		write_terminating_point();
	};


	template <typename GeometryForwardIter>
	void
	PlatesLineFormatGeometryExporter::export_feature_geometries(
			GeometryForwardIter geometries_begin,
			GeometryForwardIter geometries_end)
	{
		if (geometries_begin == geometries_end)
		{
			return;
		}

		// Export each geometry of the feature.
		GeometryForwardIter geometries_iter;
		for (geometries_iter = geometries_begin;
			geometries_iter != geometries_end;
			++geometries_iter)
		{
			// Write the coordinate list of the geometry.
			(*geometries_iter)->accept_visitor(*this);
		}

		// Write the final terminating point.
		write_terminating_point();
	}
}

#endif  // GPLATES_FILEIO_PLATESLINEFORMATGEOMETRYEXPORTER_H
