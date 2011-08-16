/* $Id$ */

/**
 * \file 
 * File formats for feature collection file readers and writers and
 * functions relating to these file formats.
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

#ifndef GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H
#define GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H

#include <boost/shared_ptr.hpp>


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		//! Formats of files that can contain feature collections.
		enum Format
		{
			GPML,              //!< '.gpml' extension.
			GPMLZ,			   //!< '.gpmlz' or '.gpml.gz' extension.
			PLATES4_LINE,      //!< '.dat' or '.pla' extension.
			PLATES4_ROTATION,  //!< '.rot' extension.
			SHAPEFILE,         //!< '.shp' extension.
			OGRGMT,			   //!< '.gmt' extension.
			WRITE_ONLY_XY_GMT,               //!< '.xy' extension.
			GMAP,              //!< '.vgp' extension.
			GSML,              //!< '.gsml' extension.

			NUM_FORMATS
		};


		/**
		 * Base class for specifying configuration options (such as for reading and/or writing a
		 * feature collection from/to a file).
		 *
		 * If a file format requires specialised options then create a derived
		 * @a Configuration class for it and register that with @a Registry.
		 */
		class Configuration
		{
		public:
			//! Typedef for a shared pointer to const @a Configuration.
			typedef boost::shared_ptr<const Configuration> shared_ptr_to_const_type;
			//! Typedef for a shared pointer to @a Configuration.
			typedef boost::shared_ptr<Configuration> shared_ptr_type;

			//! Creates a non-derived @a Configuration.
			static
			shared_ptr_type
			create(
					Format file_format)
			{
				return shared_ptr_type(new Configuration(file_format));
			}

			virtual
			~Configuration()
			{  }

			//! Returns the file format of this configuration.
			Format
			get_file_format() const
			{
				return d_file_format;
			}

		protected:
			Format d_file_format;

			explicit
			Configuration(
					Format file_format) :
				d_file_format(file_format)
			{  }
		};
	}
}

#endif // GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H
