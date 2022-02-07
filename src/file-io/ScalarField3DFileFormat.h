/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_SCALARFIELD3DFILEFORMAT_H
#define GPLATES_FILEIO_SCALARFIELD3DFILEFORMAT_H

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <QtGlobal>
#include <QDataStream>
#include <QString>

#include "global/AssertionFailureException.h"
#include "global/GPlatesException.h"

#include "utils/Endian.h"


namespace GPlatesFileIO
{
	/**
	 * This namespace contains parameters that define the GPlates 3D scalar field format.
	 *
	 * A scalar field file is a binary file that consists of a header followed by scalar field data
	 * and derived data (such as scalar field gradient).
	 *
	 * Data is stored as single-precision floating-point.
	 *
	 * The header consists of the following fields, in order:
	 *  - ( 0) A magic number that identifies a file as GPlates.
	 *  - ( 8) The file size (to check for partially written files).
	 *  - (16) The version number of the GPlates scalar field file format used.
	 *  - (20) The resolution of the tile metadata (dimensions of cube map face).
	 *  - (24) The resolution of each tile containing scalar field values and gradients (and validity mask).
	 *  - (28) The number of active tiles.
	 *  - (32) The number of depth layers.
	 *  - (36) Sequence of layer depth radii - from smallest (near globe core) to largest (near globe surface).
	 *  - (  ) Scalar minimum.
	 *  - (  ) Scalar maximum.
	 *  - (  ) Scalar mean.
	 *  - (  ) Scalar standard deviation.
	 *  - (  ) Gradient magnitude minimum.
	 *  - (  ) Gradient magnitude maximum.
	 *  - (  ) Gradient magnitude mean.
	 *  - (  ) Gradient magnitude standard deviation.
	 *
	 * Most of the fields in the header are unsigned 32-bit integers - except file offsets which are 64-bit.
	 * The byte order of the entire scalar field file is little endian (used by most hardware).
	 * The file format is independent of the operating system and CPU, with one
	 * qualification: float is assumed to be 32-bit and double is assumed to be 64-bit.
	 */
	namespace ScalarField3DFileFormat
	{
		/**
		 * The magic number that identifies a file as GPlates.
		 */
		const boost::uint8_t MAGIC_NUMBER[] = { 'G', 'P', 'l', 'a', 't', 'e', 's', 0 };

		/**
		 * The current version number of the GPlates scalar field file format.
		 *
		 * NOTE: This must be updated if there are any breaking changes to the file
		 * format between public GPlates releases.
		 * For example adding a new parameter to the file.
		 */
		const boost::uint32_t VERSION_NUMBER = 1;

		/**
		 * The QDataStream serialisation version.
		 *
		 * NOTE: We are using Qt version 4.4 data streams so the QDataStream::setFloatingPointPrecision()
		 * function is not available (introduced in Qt 4.6).
		 * So the floating-point precision written depends on stream operator called
		 * (ie, whether 'float' or 'double' is written).
		 * We are using Qt 4.4 since that is the current minimum requirement for GPlates.
		 */
		const int Q_DATA_STREAM_VERSION = QDataStream::Qt_4_4;

		/**
		 * The QDataStream byte order (most hardware is little endian so it's more efficient in general).
		 */
		const QDataStream::ByteOrder Q_DATA_STREAM_BYTE_ORDER = QDataStream::LittleEndian;

		/**
		 * Information relevant to a particular tile of data (including its depth layers).
		 */
		struct TileMetaData
		{
			/**
			 * Tile ID in half-open range [0, num_active_tiles) or it can be -1 to indicate no tile.
			 *
			 * This is stored as a float instead of integer so it can be loaded directly into a floating-point
			 * OpenGL texture. Note that floating-point can exactly represent integers up to 23 bits.
			 */
			float tile_ID;

			//! Maximum scalar value across entire tile (including all its depth layers).
			float max_scalar_value;

			//! Minimum scalar value across entire tile (including all its depth layers).
			float min_scalar_value;

			// Size of sum of individual data members.
			// This is not necessarily equal to the size of the structure due to alignment reasons.
			static const unsigned int STREAM_SIZE = 3 * sizeof(float);
		};

		/**
		 * The scalar value data (and gradient) at a particular field sample location.
		 */
		struct FieldDataSample
		{
			//! The scalar value.
			float scalar;

			//! The scalar field gradient x/y/z vector components.
			float gradient[3];

			// Size of sum of individual data members.
			// This is not necessarily equal to the size of the structure due to alignment reasons.
			static const unsigned int STREAM_SIZE = 4 * sizeof(float);
		};

		/**
		 * The mask data at a particular field sample location (x,y) location.
		 *
		 * The mask value is boolean and is 0.0 if the (x,y) sample location for all depth layers
		 * contains no scalar field data (eg, for a non-global scalar field), otherwise it's 1.0.
		 */
		struct MaskDataSample
		{
			/**
			 * The boolean mask value (0.0 or 1.0).
			 *
			 * This is stored as a float instead of integer so it can be loaded directly into a
			 * floating-point OpenGL texture.
			 */
			float mask;

			// Size of sum of individual data members.
			// This is not necessarily equal to the size of the structure due to alignment reasons.
			static const unsigned int STREAM_SIZE = 1 * sizeof(float);
		};


		/**
		 * Thrown when reading a file containing an unrecognised version number.
		 *
		 * This happens after reading the magic number so we're fairly sure it's a file that GPlates wrote.
		 *
		 * Most likely we're this is an old version of GPlates and we're reading a file generated
		 * by a newer version of GPlates.
		 */
		class UnsupportedVersion :
				public GPlatesGlobal::Exception
		{
		public:
			UnsupportedVersion(
				const GPlatesUtils::CallStack::Trace &exception_source,
				quint32 unrecognised_version);

			quint32
			unrecognised_version() const
			{
				return d_unrecognised_version;
			}

		protected:
			virtual
				const char *
				exception_name() const;

			virtual
				void
				write_message(
				std::ostream &os) const;

		private:
			quint32 d_unrecognised_version;
		};
	}
}

namespace GPlatesUtils
{
	//
	// Specialised endian-swapping functions.
	//
	namespace Endian
	{
		template<>
		inline
		void
		swap<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData>(
				GPlatesFileIO::ScalarField3DFileFormat::TileMetaData &tile_meta_data)
		{
			swap(tile_meta_data.tile_ID);
			swap(tile_meta_data.max_scalar_value);
			swap(tile_meta_data.min_scalar_value);
		}

		template<>
		inline
		void
		swap<GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample>(
				GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample &field_data_sample)
		{
			swap(field_data_sample.scalar);
			swap(field_data_sample.gradient[0]);
			swap(field_data_sample.gradient[1]);
			swap(field_data_sample.gradient[2]);
		}

		template<>
		inline
		void
		swap<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample>(
				GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample &mask_data_sample)
		{
			swap(mask_data_sample.mask);
		}
	}
}

#endif // GPLATES_FILEIO_SCALARFIELD3DFILEFORMAT_H
