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

#include <boost/shared_ptr.hpp>
#include <QMap>
#include <QString>

#include "FeatureCollectionFileFormatConfiguration.h"
#include "FeatureCollectionFileFormatRegistry.h"
#include "GMTFormatWriter.h"


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
		 * Configuration options for the OGRGMT and SHAPEFILE file formats.
		 */
		class OGRConfiguration :
				public Configuration
		{
		public:
			typedef boost::shared_ptr<const OGRConfiguration> shared_ptr_to_const_type;
			typedef boost::shared_ptr<OGRConfiguration> shared_ptr_type;

			//! Typedef for a model-to-attribute mapping.
			typedef QMap<QString, QString> model_to_attribute_map_type;

			/**
			 * Constructor - the default model-to-attribute map is empty.
			 *
			 * NOTE: @a file_format must currently be either OGRGMT or SHAPEFILE.
			 *
			 * @a wrap_to_dateline enables wrapping of polyline/polygon geometries to dateline (disabled by default).
			 */
			explicit
			OGRConfiguration(
					Format file_format,
					bool wrap_to_dateline = false,
					const model_to_attribute_map_type model_to_shapefile_map = model_to_attribute_map_type()) :
				d_wrap_to_dateline(wrap_to_dateline),
				d_model_to_attribute_map(model_to_shapefile_map)
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


			//! Returns the 'const' model-to-attribute map.
			const model_to_attribute_map_type &
			get_model_to_attribute_map() const
			{
				return d_model_to_attribute_map;
			}

			//! Returns the 'non-const' model-to-attribute map.
			model_to_attribute_map_type &
			get_model_to_attribute_map()
			{
				return d_model_to_attribute_map;
			}

		private:
			bool d_wrap_to_dateline;
			model_to_attribute_map_type d_model_to_attribute_map;
		};
	}
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONFILEFORMATCONFIGURATIONS_H
