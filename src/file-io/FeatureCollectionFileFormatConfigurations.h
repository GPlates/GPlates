/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATIONS_H
#define GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATIONS_H

#include <string>
#include <boost/shared_ptr.hpp>
#include <QMap>
#include <QString>

#include "FeatureCollectionFileFormatConfiguration.h"
#include "FeatureCollectionFileFormatRegistry.h"
#include "GMTFormatWriter.h"

#include "model/FeatureCollectionHandle.h"
#include "property-values/SpatialReferenceSystem.h"


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		/**
		 * Configuration options for the write-only GMT format 'WRITE_ONLY_XY_GMT'.
		 */
		class GMTConfiguration :
				public Configuration
		{
		public:
			typedef boost::shared_ptr<const GMTConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<GMTConfiguration> shared_ptr_type;

			//! Constructor.
			explicit
			GMTConfiguration(
					GPlatesFileIO::GMTFormatWriter::HeaderFormat header_format =
							GPlatesFileIO::GMTFormatWriter::PLATES4_STYLE_HEADER) :
				d_header_format(header_format)
			{  }

			//! Returns the GMT header format.
			GPlatesFileIO::GMTFormatWriter::HeaderFormat
			get_header_format() const
			{
				return d_header_format;
			}

			//! Sets the GMT header format.
			void
			set_header_format(
					GPlatesFileIO::GMTFormatWriter::HeaderFormat header_format)
			{
				d_header_format = header_format;
			}

		private:
			GPlatesFileIO::GMTFormatWriter::HeaderFormat d_header_format;
		};


		/**
		 * Configuration options for OGR-supported file formats.
		 */
		class OGRConfiguration :
				public Configuration
		{
		public:
			typedef boost::shared_ptr<const OGRConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<OGRConfiguration> shared_ptr_type;

			enum OgrSrsWriteBehaviour
			{
				WRITE_AS_WGS84_BEHAVIOUR,
				WRITE_AS_ORIGINAL_SRS_BEHAVIOUR
			};

			//! Typedef for a model-to-attribute mapping.
			typedef QMap<QString, QString> model_to_attribute_map_type;


			/**
			 * Constructor.
			 *
			 * NOTE: @a file_format must currently be one of:
			 * OGRGMT
			 * SHAPEFILE
			 * GEOJSON
			 * GEOPACKAGE
			 *
			 * @a wrap_to_dateline enables wrapping of polyline/polygon geometries to dateline.
			 */
			explicit
			OGRConfiguration(
					Format file_format,
					bool wrap_to_dateline) :
				d_wrap_to_dateline(wrap_to_dateline),
				d_ogr_srs_write_behaviour(WRITE_AS_WGS84_BEHAVIOUR)
			{  }


			//! Returns dateline wrapping flag.
			bool
			get_wrap_to_dateline() const
			{
				return d_wrap_to_dateline;
			}

			//! Sets dateline wrapping flag.
			void
			set_wrap_to_dateline(
					bool wrap_to_dateline)
			{
				d_wrap_to_dateline = wrap_to_dateline;
			}

			OgrSrsWriteBehaviour
			get_ogr_srs_write_behaviour() const
			{
				return d_ogr_srs_write_behaviour;
			}

			void
			set_ogr_srs_write_behaviour(
					const OgrSrsWriteBehaviour &behaviour)
			{
				d_ogr_srs_write_behaviour = behaviour;
			}


			/**
			 * Returns the model-to-attribute map.
			 *
			 * NOTE: The model-to-attribute map is no longer stored in the file configuration,
			 * but in the feature collection itself (as a tag).
			 * This ensures the mapping is retained when the feature collection gets separated
			 * from its file container.
			 * Also the model-to-attribute map is persistent (stored in shapefile mapping file)
			 * whereas the file configuration parameters specified by the user within GPlates and
			 * not stored to disk.
			 */
			static
			model_to_attribute_map_type &
			get_model_to_attribute_map(
					GPlatesModel::FeatureCollectionHandle &feature_collection);

			/**
			 * @brief get_original_file_srs
			 * @return the original SRS of the OGR data source, if one was provided.
			 */
			boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
			get_original_file_srs() const;

			/**
			 * @brief set_original_file_srs
			 * Sets the original SRS of the OGR data source.
			 */
			void
			set_original_file_srs(
					const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type &srs);

		private:

			/**
			 * The key string used when storing the model-to-attribute map as a tag in a FeatureCollectionHandle.
			 */
			static const std::string FEATURE_COLLECTION_TAG;


			bool d_wrap_to_dateline;

			/**
			 * @brief d_original_file_srs - The original SRS of the OGR data source, if one was provided.
			 */
			boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
				d_original_file_srs;

			/**
			 * @brief d_ogr_srs_write_behaviour - enum for controlling how the SRS is handled on output.
			 */
			OgrSrsWriteBehaviour d_ogr_srs_write_behaviour;


		};
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATIONS_H
