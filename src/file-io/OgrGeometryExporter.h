/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2011 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEGEOMETRYEXPORTER_H
#define GPLATES_FILEIO_SHAPEFILEGEOMETRYEXPORTER_H

#include <boost/noncopyable.hpp>

#include <QFile>

#include "GeometryExporter.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"


namespace GPlatesFileIO
{
	class OgrWriter;

	class OgrGeometryExporter :
		public GPlatesMaths::ConstGeometryOnSphereVisitor,
		public GeometryExporter,
		private boost::noncopyable
	{
	public:

		OgrGeometryExporter(
			QString &filename,
			bool multiple_geometries);

		virtual
			~OgrGeometryExporter();

		virtual
		void
		export_geometry(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr);

		virtual
		void
		export_geometry(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type key_value_dictionary);


	private:

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



		QString d_filename;

		bool d_multiple_geometries;

		OgrWriter *d_ogr_writer;

		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type>
			d_key_value_dictionary;

	};

}

#endif // GPLATES_FILEIO_SHAPEFILEGEOMETRYEXPORTER_H
