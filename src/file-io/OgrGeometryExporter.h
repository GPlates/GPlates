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

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <QFile>

#include "GeometryExporter.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "property-values/GpmlKeyValueDictionary.h"


namespace GPlatesFileIO
{
	class OgrWriter;

	class OgrGeometryExporter :
		public GPlatesMaths::ConstGeometryOnSphereVisitor,
		public GeometryExporter,
		private boost::noncopyable
	{
	public:

		/**
		 * If all the geometry types to be written are not the same type then @a multiple_geometry_types
		 * should be set to true (this will create multiple exported files - one per geometry type encountered).
		 */
		OgrGeometryExporter(
			QString &filename,
			bool multiple_geometry_types,
			bool wrap_to_dateline = true);

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

		/**
		 * Export a sequence of geometries.
		 *
		 * This is useful if multiple geometries should be written out as a single feature.
		 *
		 * However different geometry types will still need to go into separate features/files but if
		 * the geometries are the same type then they will get written out as a single feature with
		 * multi-part geometry. Also see constructor's 'multiple_geometry_types' argument.
		 *
		 * Also turns out that the OGR Shapefile driver can combine two polygon geometries into
		 * a single polygon with an exterior and interior ring (provided one polygon is fully
		 * contained inside the other - ie, if they don't intersect) and clockwise orient the exterior
		 * ring and counter-clockwise orient the interior rings. We shouldn't rely on this though
		 * since we support exterior and interior rings inside PolygonOnSphere which we pass to OGR.
		 */
		template <typename ForwardGeometryIter>
		void
		export_geometries(
			ForwardGeometryIter geometries_begin,
			ForwardGeometryIter geometries_end,
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> key_value_dictionary);


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


		void
		clear_geometries();

		void
		write_geometries();


		QString d_filename;

		OgrWriter *d_ogr_writer;

		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type>
			d_key_value_dictionary;

		// Store various geometries encountered in each feature. 
		std::vector<GPlatesMaths::PointOnSphere> d_point_geometries;
		std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> d_multi_point_geometries;
		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_polyline_geometries;
		std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_geometries;

	};

}

//
// Implementation
//

namespace GPlatesFileIO
{
	template <typename ForwardGeometryIter>
	void
	OgrGeometryExporter::export_geometries(
			ForwardGeometryIter geometries_begin,
			ForwardGeometryIter geometries_end,
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> key_value_dictionary)
	{
		d_key_value_dictionary = key_value_dictionary;
		clear_geometries();

		// Visit each geometry in the sequence.
		for (ForwardGeometryIter geometries_iter = geometries_begin;
			geometries_iter != geometries_end;
			++geometries_iter)
		{
			(*geometries_iter)->accept_visitor(*this);
		}

		write_geometries();
	}
}

#endif // GPLATES_FILEIO_SHAPEFILEGEOMETRYEXPORTER_H
