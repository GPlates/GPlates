/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEWRITER_H
#define GPLATES_FILEIO_SHAPEFILEWRITER_H

#include <iosfwd>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QFile>
#include <QString>

#include "FileInfo.h"
#include "OgrWriter.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"
#include "model/ConstFeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/PropertyName.h"


namespace GPlatesFileIO
{

	class ShapefileWriter :
		public GPlatesModel::ConstFeatureVisitor
	{
	public:

		/**
		* @pre is_writable(file_info) is true.
		* @param file_info file to write to.
		* @param feature_collection_ref feature collection that will be written
		*        using calls to @a write_feature.
		*/
		explicit
		ShapefileWriter(
				const FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_ref);

		virtual
			~ShapefileWriter()
		{};

	private:

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
			void
			visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
			void
			visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
			void
			visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
			void
			visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
			void
			visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
			void
			visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
			void
			visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		/**
		 * Clears the various geometry accumulators.                                                                      
		 */
		void
		clear_accumulators();

		boost::scoped_ptr<QFile> d_output_file;

		// The first GpmlKeyValueDictionary encountered while traversing a feature.
		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> d_key_value_dictionary;

		// A default KeyValueDictionary used for features for which no KVD is found.
		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> d_default_key_value_dictionary;

		// A model_to_shapefile_attribute map
		QMap< QString,QString > d_model_to_shapefile_map;

		boost::scoped_ptr<OgrWriter> d_ogr_writer;

		// Store various geometries encountered in each feature. 
		std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> d_point_geometries;
		std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> d_multi_point_geometries;
		std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_polyline_geometries;
		std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon_geometries;

	};
}

#endif // GPLATES_FILEIO_SHAPEFILEWRITER_H
