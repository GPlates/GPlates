/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEREADER_H
#define GPLATES_FILEIO_SHAPEFILEREADER_H

#include "ogrsf_frmts.h"

#include "FileInfo.h"
#include "ReadErrorAccumulation.h"

#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelUtils.h"
#include "model/DummyTransactionHandle.h"

namespace GPlatesFileIO {
	
	const double SHAPE_NO_DATA = -1e38; 

	class ShapeFileReader
	{

	public:

		static
		const GPlatesModel::FeatureCollectionHandle::weak_ref
		read_file(
				FileInfo& fileinfo,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);

	private:

		ShapeFileReader();

		~ShapeFileReader();

// Make copy constructor private
		ShapeFileReader(
			const ShapeFileReader &other);


// Make assignment private
		ShapeFileReader &
		operator=(
				const ShapeFileReader &other);


		bool
		check_file_format(
				ReadErrorAccumulation &read_errors);

		bool 
		open_file(
				const QString &filename);

		void 
		get_field_names(
			ReadErrorAccumulation &read_errors);

		void 
		get_attributes();

		void
		handle_point(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_multi_point(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);


		void
		handle_linestring(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
				);

		void
		handle_multi_linestring(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_polygon(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_multi_polygon(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::FeatureCollectionHandle::weak_ref collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);


		const 
		GPlatesModel::FeatureCollectionHandle::weak_ref
		read_features(
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);

		GPlatesModel::FeatureHandle::weak_ref
		create_line_feature_from_list(
				GPlatesModel::ModelInterface&,
				GPlatesModel::FeatureCollectionHandle::weak_ref,
				std::list<GPlatesMaths::PointOnSphere> &list);

		GPlatesModel::FeatureHandle::weak_ref
		create_point_feature_from_pair(
			GPlatesModel::ModelInterface&,
			GPlatesModel::FeatureCollectionHandle::weak_ref,
			std::pair<double,double> &pair);

		GPlatesModel::FeatureHandle::weak_ref
		create_point_feature_from_point_on_sphere(
			GPlatesModel::ModelInterface&,
			GPlatesModel::FeatureCollectionHandle::weak_ref,
			GPlatesMaths::PointOnSphere &point);


		void
		add_attributes_to_feature(
			GPlatesModel::FeatureHandle::weak_ref,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		bool 
		is_valid_shape_data(
			double lat,
			double lon,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
			);

		OGRwkbGeometryType 
		get_OGR_type();
				
		QString d_filename;

		int d_num_layers;

		OGRDataSource *d_data_source_ptr;

		OGRGeometry *d_geometry_ptr;

		OGRFeature *d_feature_ptr;

		OGRLayer *d_layer_ptr;

		OGRwkbGeometryType d_type;

		std::map<int, QString> d_field_names;

		std::vector<QVariant> d_attributes;

	};


}

#endif  // GPLATES_FILEIO_SHAPEFILEREADER_H
