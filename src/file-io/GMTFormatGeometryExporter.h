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

#ifndef GPLATES_FILEIO_GMTFORMATGEOMETRYEXPORTER_H
#define GPLATES_FILEIO_GMTFORMATGEOMETRYEXPORTER_H

#include <boost/noncopyable.hpp>
#include <QTextStream>

#include "GeometryExporter.h"

#include "maths/ConstGeometryOnSphereVisitor.h"


namespace GPlatesFileIO
{
	/**
	* This class is a ConstGeometryOnSphereVisitor which will output GMT xy
	* points format for the geometry it visits.
	*
	* See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	* operation of this class.  This class corresponds to the abstract Visitor class in the
	* pattern structure.
	*/
	class GMTFormatGeometryExporter :
			public GPlatesMaths::ConstGeometryOnSphereVisitor,
			public GeometryExporter,
			private boost::noncopyable
	{
	public:

		GMTFormatGeometryExporter(
				QTextStream &output_stream,
				bool reverse_coordinate_order = false,
				bool polygon_terminating_point = true);

		virtual
		~GMTFormatGeometryExporter()
		{  }


		/**
		* You should call this method on the geometry you wish to write,
		* rather than directly calling ->accept_visitor(*this) on the geometry,
		* since we need to write the final terminating marker. (and possibly a
		* dummy header?)
		*/
		virtual
		void
		export_geometry(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr);

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

	private:

		/**
		* The QTextStream we write to.
		* QTextStreams can conveniently be created from QIODevices and QByteArrays.
		*/
		QTextStream *d_stream_ptr;

		/**
		* Should we go against the norm and write out coordinates using a (lat,lon) ordering?
		*/
		bool d_reverse_coordinate_order;

		/**
		* Should we convert gml:Polygons to something the GMT xy format can render,
		* by adding an additional terminating point identical to the first point?
		*/
		bool d_polygon_terminating_point;

	};

}

#endif // GPLATES_FILEIO_GMTFORMATGEOMETRYEXPORTER_H
