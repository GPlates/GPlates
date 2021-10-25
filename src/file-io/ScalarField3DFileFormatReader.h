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

#ifndef GPLATES_FILEIO_SCALARFIELD3DFILEFORMATREADER_H
#define GPLATES_FILEIO_SCALARFIELD3DFILEFORMATREADER_H

#include <utility>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QtGlobal>

#include "ScalarField3DFileFormat.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/Profile.h"


namespace GPlatesFileIO
{
	namespace ScalarField3DFileFormat
	{
		/**
		 * Reads 3D scalar field data from a file.
		 */
		class Reader :
				private boost::noncopyable
		{
		public:

			/**
			 * Opens @a filename for reading as a 3D scalar field file.
			 *
			 * @throws @a ErrorOpeningFileForReadingException if @a filename could not be opened for reading.
			 *
			 * @throws @a FileFormatNotSupportedException if the header information is wrong.
			 *
			 * @throws @a ScalarField3DFileFormat::UnsupportedVersion if the version is either not recognised
			 * (file created by a newer version of GPlates) or no longer supported (eg, if format
			 * is an old format that is inefficient and hence should be regenerated with a newer algorithm).
			 */
			Reader(
					const QString &filename);


			/**
			 * Returns the resolution of the cube texture containing tile metadata.
			 */
			unsigned int
			get_tile_meta_data_resolution() const
			{
				return d_impl->get_tile_meta_data_resolution();
			}


			/**
			 * Returns the tile resolution of tiles containing field data (and mask data).
			 */
			unsigned int
			get_tile_resolution() const
			{
				return d_impl->get_tile_resolution();
			}


			/**
			 * Returns the number of active tiles.
			 */
			unsigned int
			get_num_active_tiles() const
			{
				return d_impl->get_num_active_tiles();
			}


			/**
			 * Returns the number of depth layers for the tiles containing field data.
			 */
			unsigned int
			get_num_depth_layers_per_tile() const
			{
				return d_impl->get_num_depth_layers_per_tile();
			}


			/**
			 * Returns the radius of each depth layer.
			 *
			 * These assume the Earth has unit radius.
			 * Deeper layers (closer to the Earth's core) have smaller radii.
			 * Layers are ordered from smaller to larger radii (from Earth centre towards surface).
			 */
			const std::vector<float> &
			get_depth_layer_radii() const
			{
				return d_impl->get_depth_layer_radii();
			}


			/**
			 * Returns the minimum depth layer radius.
			 */
			float
			get_minimum_depth_layer_radius() const
			{
				return d_impl->get_depth_layer_radii().front();
			}

			/**
			 * Returns the maximum depth layer radius.
			 */
			float
			get_maximum_depth_layer_radius() const
			{
				return d_impl->get_depth_layer_radii().back();
			}


			/**
			 * Returns the total number of layers across all tiles.
			 */
			unsigned int
			get_num_layers() const
			{
				return get_num_active_tiles() * get_num_depth_layers_per_tile();
			}


			/**
			 * Returns the minimum scalar value across the entire scalar field.
			 */
			double
			get_scalar_min() const
			{
				return d_impl->get_scalar_min();
			}

			/**
			 * Returns the maximum scalar value across the entire scalar field.
			 */
			double
			get_scalar_max() const
			{
				return d_impl->get_scalar_max();
			}

			/**
			 * Returns the mean scalar value across the entire scalar field.
			 */
			double
			get_scalar_mean() const
			{
				return d_impl->get_scalar_mean();
			}

			/**
			 * Returns the standard deviation of scalar values across the entire scalar field.
			 */
			double
			get_scalar_standard_deviation() const
			{
				return d_impl->get_scalar_standard_deviation();
			}


			/**
			 * Returns the minimum gradient magnitude across the entire scalar field.
			 */
			double
			get_gradient_magnitude_min() const
			{
				return d_impl->get_gradient_magnitude_min();
			}

			/**
			 * Returns the maximum gradient magnitude across the entire scalar field.
			 */
			double
			get_gradient_magnitude_max() const
			{
				return d_impl->get_gradient_magnitude_max();
			}

			/**
			 * Returns the mean gradient magnitude across the entire scalar field.
			 */
			double
			get_gradient_magnitude_mean() const
			{
				return d_impl->get_gradient_magnitude_mean();
			}

			/**
			 * Returns the standard deviation of gradient magnitude across the entire scalar field.
			 */
			double
			get_gradient_magnitude_standard_deviation() const
			{
				return d_impl->get_gradient_magnitude_standard_deviation();
			}


			/**
			 * Reads the tile metadata.
			 *
			 * The order of six cube faces is the order of enums in 'GPlatesMaths::CubeCoordinateFrame::CubeFaceType'.
			 *
			 * Within each face the data is laid out as rows from negative-to-positive Y and each row
			 * is negative-to-positive X, where X and Y are the local coordinate axes defined by
			 * 'GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis()'.
			 *
			 * Each cube face has resolution @a get_tile_meta_data_resolution.
			 *
			 * @throws FileFormatNotSupportedException on error.
			 */
			boost::shared_array<TileMetaData>
			read_tile_meta_data() const
			{
				return d_impl->read_tile_meta_data();
			}


			/**
			 * Reads the tile field data (scalar/gradient field samples).
			 *
			 * The data is arranged as a sequence of 2D tile images where the first @a get_num_depth_layers_per_tile
			 * images are associated with the first active tile, etc, giving a total of
			 * "get_num_active_tiles * get_num_depth_layers_per_tile" 2D images where each square image has
			 * a resolution of @a get_tile_resolution.
			 *
			 * Within each image the data is laid out as rows from negative-to-positive Y and each row
			 * is negative-to-positive X, where X and Y are the local coordinate axes defined by
			 * 'GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis()' where each
			 * image, or tile, belongs to a particular cube face.
			 *
			 * @param layer_index index into 'get_num_layers' layers to start reading at.
			 * @param num_layers_to_read is the total number of layers to read.
			 *
			 * To read entire field data in one call use...
			 *   layer_index = 0
			 *   num_layers_to_read = get_num_layers
			 * ...for the function arguments.
			 *
			 * @throws FileFormatNotSupportedException on error.
			 */
			boost::shared_array<FieldDataSample>
			read_field_data(
					unsigned int layer_index,
					unsigned int num_layers_to_read) const
			{
				return d_impl->read_field_data(layer_index, num_layers_to_read);
			}


			/**
			 * Reads the tile mask data (determines which areas of field data contain valid data used for non-global fields).
			 *
			 * The data is arranged as a sequence of 2D tile images with one mask image associated with
			 * each tile giving a total of "get_num_active_tiles" 2D images where each square image has
			 * a resolution of @a get_tile_resolution.
			 *
			 * Within each image the data is laid out as rows from negative-to-positive Y and each row
			 * is negative-to-positive X, where X and Y are the local coordinate axes defined by
			 * 'GPlatesMaths::CubeCoordinateFrame::get_cube_face_coordinate_frame_axis()' where each
			 * image, or tile, belongs to a particular cube face.
			 *
			 * @param tile_index index into 'get_num_active_tiles' mask tiles to start reading at.
			 * @param num_tiles_to_read is the total number of mask tiles to read.
			 *
			 * To read entire mask data in one call use...
			 *   tile_index = 0
			 *   num_tiles_to_read = get_num_active_tiles
			 * ...for the function arguments.
			 *
			 * @throws FileFormatNotSupportedException on error.
			 */
			boost::shared_array<MaskDataSample>
			read_mask_data(
					unsigned int tile_index,
					unsigned int num_tiles_to_read) const
			{
				return d_impl->read_mask_data(tile_index, num_tiles_to_read);
			}


			/**
			 * Retrieves information about the file that we are reading.
			 */
			QFileInfo
			get_file_info() const
			{
				return QFileInfo(d_file);
			}


			/**
			 * Returns the filename of the file that we are reading.
			 */
			QString
			get_filename() const
			{
				return d_file.fileName();
			}


		private:

			class ReaderImpl
			{
			public:

				virtual
				~ReaderImpl()
				{  }

				virtual
				unsigned int
				get_tile_meta_data_resolution() const = 0;

				virtual
				unsigned int
				get_tile_resolution() const = 0;

				virtual
				unsigned int
				get_num_active_tiles() const = 0;

				virtual
				unsigned int
				get_num_depth_layers_per_tile() const = 0;

				virtual
				const std::vector<float> &
				get_depth_layer_radii() const = 0;

				virtual
				double
				get_scalar_min() const = 0;

				virtual
				double
				get_scalar_max() const = 0;

				virtual
				double
				get_scalar_mean() const = 0;

				virtual
				double
				get_scalar_standard_deviation() const = 0;

				virtual
				double
				get_gradient_magnitude_min() const = 0;

				virtual
				double
				get_gradient_magnitude_max() const = 0;

				virtual
				double
				get_gradient_magnitude_mean() const = 0;

				virtual
				double
				get_gradient_magnitude_standard_deviation() const = 0;

				virtual
				boost::shared_array<TileMetaData>
				read_tile_meta_data() const = 0;

				virtual
				boost::shared_array<FieldDataSample>
				read_field_data(
						unsigned int layer_index,
						unsigned int num_layers_to_read) const = 0;

				virtual
				boost::shared_array<MaskDataSample>
				read_mask_data(
						unsigned int tile_index,
						unsigned int num_tiles_to_read) const = 0;
			};


			/**
			 * A reader for version 1+ files.
			 */
			class VersionOneReader :
					public ReaderImpl
			{
			public:

				VersionOneReader(
						quint32 version_number,
						QFile &file,
						QDataStream &in);

				~VersionOneReader()
				{  }

				virtual
				unsigned int
				get_tile_meta_data_resolution() const
				{
					return d_tile_meta_data_resolution;
				}

				virtual
				unsigned int
				get_tile_resolution() const
				{
					return d_tile_resolution;
				}

				virtual
				unsigned int
				get_num_active_tiles() const
				{
					return d_num_active_tiles;
				}

				virtual
				unsigned int
				get_num_depth_layers_per_tile() const
				{
					return d_num_depth_layers;
				}

				virtual
				const std::vector<float> &
				get_depth_layer_radii() const
				{
					return d_depth_layer_radii;
				}

				virtual
				double
				get_scalar_min() const
				{
					return d_scalar_min;
				}

				virtual
				double
				get_scalar_max() const
				{
					return d_scalar_max;
				}

				virtual
				double
				get_scalar_mean() const
				{
					return d_scalar_mean;
				}

				virtual
				double
				get_scalar_standard_deviation() const
				{
					return d_scalar_standard_deviation;
				}

				virtual
				double
				get_gradient_magnitude_min() const
				{
					return d_gradient_magnitude_min;
				}

				virtual
				double
				get_gradient_magnitude_max() const
				{
					return d_gradient_magnitude_max;
				}

				virtual
				double
				get_gradient_magnitude_mean() const
				{
					return d_gradient_magnitude_mean;
				}

				virtual
				double
				get_gradient_magnitude_standard_deviation() const
				{
					return d_gradient_magnitude_standard_deviation;
				}

				virtual
				boost::shared_array<TileMetaData>
				read_tile_meta_data() const;

				virtual
				boost::shared_array<FieldDataSample>
				read_field_data(
						unsigned int layer_index,
						unsigned int num_layers_to_read) const;

				virtual
				boost::shared_array<MaskDataSample>
				read_mask_data(
						unsigned int tile_index,
						unsigned int num_tiles_to_read) const;

			private:

				QFile &d_file;
				QDataStream &d_in;

				unsigned int d_tile_meta_data_resolution;
				unsigned int d_tile_resolution;
				unsigned int d_num_active_tiles;
				unsigned int d_num_depth_layers;
				std::vector<float> d_depth_layer_radii;

				double d_scalar_min;
				double d_scalar_max;
				double d_scalar_mean;
				double d_scalar_standard_deviation;

				double d_gradient_magnitude_min;
				double d_gradient_magnitude_max;
				double d_gradient_magnitude_mean;
				double d_gradient_magnitude_standard_deviation;

				qint64 d_field_data_file_offset;
				qint64 d_mask_data_file_offset;
				qint64 d_tile_meta_data_file_offset;
			};


			QFile d_file;
			QDataStream d_in;
			boost::scoped_ptr<ReaderImpl> d_impl;
		};
	}
}

#endif // GPLATES_FILEIO_SCALARFIELD3DFILEFORMATREADER_H
