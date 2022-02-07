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

#ifndef GPLATES_FILEIO_OGRWRITER_H
#define GPLATES_FILEIO_OGRWRITER_H

#include <vector>
#include <QDebug>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GdalUtils.h"
#include "FeatureCollectionFileFormatConfigurations.h"
#include "Ogr.h"

#include "maths/DateLineWrapper.h"
#include "maths/LatLonPoint.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/CoordinateTransformation.h"
#include "property-values/SpatialReferenceSystem.h"


namespace GPlatesFileIO
{

	/**
	 * Uses the OGR library to write geometries and attributes to OGR-supported file formats.                                                                    
	 */
	class OgrWriter 
	{
	public:

		/**
		 * @a filename: target filename for output.
		 * @a multiple_layers: whether or not the feature of feature collections to be written contain
		 * multiple geometry-types. 
		 * @a wrap_to_dateline whether to wrap/clip polyline/polygon geometries to the dateline (for ArcGIS viewing).
		 *
		 * Multiple geometry types will be exported to a subfolder of name @a filename (less the file extension).
		 */
		OgrWriter(
			QString filename,
			bool multiple_layers,
			bool wrap_to_dateline = true,
			boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> original_srs = boost::none,
			const GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::OgrSrsWriteBehaviour &behaviour =
				GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::WRITE_AS_WGS84_BEHAVIOUR);

		~OgrWriter();

		/**
		 * Write a point feature to a point-type layer. If a point-type layer does not exist, it will
		 * be created if possible. 
		 */
		void
		write_point_feature(
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);

		void
		write_multi_point_feature(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);		

		void
		write_polyline_feature(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	

		void
		write_multi_polyline_feature(
			const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polyline_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	

		void
		write_polygon_feature(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);	
			
		void
		write_multi_polygon_feature(
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygon_on_sphere,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);				

	private:
		/**
		 * The OGR driver.  
		 *
		 * We have to instantiate a driver of the appropriate type (ESRI shapefile, OGR-GMT etc) before
		 * we can create output files. 
		 */
		GdalUtils::vector_data_driver_type *d_ogr_driver_ptr;


		/**
		 * Filename used by Ogr library to create a data source.
		 */
		QString d_filename;

		/**
		 * Filename stripped of any extension for use in naming layers. 
		 */
		QString d_layer_basename;

		/**
		 * File extension                                                                    
		 */
		QString d_extension;

		/**
		 *  True if the feature-collection/feature contains more than one geometry type.                                                                    
		 */
		bool d_multiple_geometry_types;

		/**
		 * True if polyline/polygon geometries should be wrapped (clipped) to the dateline (for ArcGIS viewing).
		 */
		bool d_wrap_to_dateline;

		GdalUtils::vector_data_source_type *d_ogr_data_source_ptr;

		// Data source for each of the geometry types. 
		GdalUtils::vector_data_source_type *d_ogr_point_data_source_ptr;
		GdalUtils::vector_data_source_type *d_ogr_line_data_source_ptr;
		GdalUtils::vector_data_source_type *d_ogr_polygon_data_source_ptr;

		/**
		 * Pointers to the geometry layers. Not all geometry layers will be required, hence they're
		 * optional.  These layer pointers don't have ownership of the layer. 
		 */
		boost::optional<OGRLayer*> d_ogr_point_layer;
		boost::optional<OGRLayer*> d_ogr_multi_point_layer;
		boost::optional<OGRLayer*> d_ogr_polyline_layer;
		boost::optional<OGRLayer*> d_ogr_polygon_layer;

		/**
		 * Used to wrap/clip polyline/polygon geometries to the dateline (if enabled).
		 */
		GPlatesMaths::DateLineWrapper::non_null_ptr_type d_dateline_wrapper;

		/**
		 * SRS of the original feature collection (if appropriate, i.e. if the collection we are writing was derived
		 * from an OGR-compatible source which provided an SRS).
		 */
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> d_original_srs;

		FeatureCollectionFileFormat::OGRConfiguration::OgrSrsWriteBehaviour d_ogr_srs_write_behaviour;

		/**
		 * @brief d_current_coordinate_transformation - The coordinate transformation from WGS84 to the original SRS.
		 */
		GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type d_coordinate_transformation;

		/**
		 * Common method to write a single polyline or multiple polylines.
		 */
		void
		write_single_or_multi_polyline_feature(
			const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);

		/**
		 * Common method to write a single polygon or multiple polygons.
		 */
		void
		write_single_or_multi_polygon_feature(
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary);
	};

}

#endif // GPLATES_FILEIO_OGRWRITER_H
