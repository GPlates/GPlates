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

#include <QSysInfo>

#include "ScalarField3DFileFormatReader.h"

#include "ErrorOpeningFileForReadingException.h"
#include "FileFormatNotSupportedException.h"

#include "global/LogException.h"
#include "global/PreconditionViolationError.h"


GPlatesFileIO::ScalarField3DFileFormat::Reader::Reader(
		const QString &filename) :
	d_file(filename),
	d_in(&d_file)
{
	// Attempt to open the file for reading.
	if (!d_file.open(QIODevice::ReadOnly))
	{
		throw ErrorOpeningFileForReadingException(
				GPLATES_EXCEPTION_SOURCE, filename);
	}

	d_in.setVersion(Q_DATA_STREAM_VERSION);
	d_in.setByteOrder(Q_DATA_STREAM_BYTE_ORDER);

	// Check that there is enough data in the file for magic number, file size and version.
	QFileInfo file_info(d_file);
	static const qint64 MAGIC_NUMBER_AND_FILE_SIZE_AND_VERSION_SIZE =
			sizeof(MAGIC_NUMBER) + sizeof(qint64) + sizeof(quint32);
	if (file_info.size() < MAGIC_NUMBER_AND_FILE_SIZE_AND_VERSION_SIZE)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad header in scalar field file");
	}

	// Check the magic number.
	for (unsigned int n = 0; n < sizeof(MAGIC_NUMBER); ++n)
	{
		quint8 magic_number;
		d_in >> magic_number;
		if (magic_number != MAGIC_NUMBER[n])
		{
			throw FileFormatNotSupportedException(
					GPLATES_EXCEPTION_SOURCE, "bad magic number in scalar field file");
		}
	}

	// The size of the file so we can check with the actual size.
	qint64 total_file_size;
	d_in >> total_file_size;
#if 0
	qDebug() << "total_file_size " << total_file_size;
	qDebug() << "file_info.size() " << file_info.size();
#endif

	// Check that the file length is correct.
	// This is in case scalar field file generation from a previous instance of GPlates failed
	// part-way through writing the file and didn't remove the file for some reason.
	if (total_file_size != static_cast<qint64>(file_info.size()))
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "detected a partially written scalar field file");
	}

	// Check the version number.
	quint32 version_number;
	d_in >> version_number;

	// Determine which reader to use depending on the version.
	//
	// TODO: Change version 0 to version 1 once initial development/debug/testing complete.
	// Then all version 0 files used during development can no longer be used.
	// This is so we don't continually increment the version number as we make changes to the file
	// format during initial development.
	if (version_number == 0)
	{
		d_impl.reset(new VersionOneReader(version_number, d_file, d_in));
	}
	// The following demonstrates a possible future scenario where VersionOneReader is used
	// for versions 1 and 2 and VersionsThreeReader is used for versions 3, 4, 5.
	// This could happen if only a small change is needed for version 2 but a larger more
	// structural change is required at version 3 necessitating a new reader class...
#if 0
	else if (version_number >=3 && version_number <= VERSION_NUMBER)
	{
		d_impl.reset(new VersionThreeReader(version_number, d_file, d_in));
	}
#endif
	else
	{
		throw UnsupportedVersion(
				GPLATES_EXCEPTION_SOURCE, version_number);
	}
}


GPlatesFileIO::ScalarField3DFileFormat::Reader::VersionOneReader::VersionOneReader(
		quint32 version_number,
		QFile &file,
		QDataStream &in) :
	d_file(file),
	d_in(in),
	d_tile_meta_data_resolution(0),
	d_tile_resolution(0),
	d_num_active_tiles(0),
	d_num_depth_layers(0),
	d_tile_meta_data_file_offset(0),
	d_field_data_file_offset(0),
	d_mask_data_file_offset(0)
{
	// NOTE: The total file size has been verified before we get here so there's no
	// need to check that the file is large enough to read data as we read.

	// Read the tile metadata cube resolution.
	quint32 tile_meta_data_resolution;
	d_in >> tile_meta_data_resolution;
	d_tile_meta_data_resolution = tile_meta_data_resolution;
	// Check the number to see if it's reasonable.
	if (tile_meta_data_resolution == 0 ||
		tile_meta_data_resolution > 1024/*arbitrary*/)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad tile metadata resolution in scalar field file");
	}

	// Read the tile resolution (resolution of each data tile).
	quint32 tile_resolution;
	d_in >> tile_resolution;
	d_tile_resolution = tile_resolution;
	// Check the number to see if it's reasonable.
	if (tile_resolution == 0 ||
		tile_resolution > 256 * 1024/*arbitrary*/)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad tile resolution in scalar field file");
	}

	// Read the number of active tiles.
	quint32 num_active_tiles;
	d_in >> num_active_tiles;
	d_num_active_tiles = num_active_tiles;
	// Check the number to see if it's reasonable.
	if (num_active_tiles < 6/*at least one per cube face*/ ||
		num_active_tiles > 1024 * 1024/*arbitrary*/)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad number of active tiles in scalar field file");
	}

	// Read the number of depth layers.
	quint32 num_depth_layers;
	d_in >> num_depth_layers;
	d_num_depth_layers = num_depth_layers;
	// Check the number to see if it's reasonable.
	if (num_depth_layers < 2 ||
		num_depth_layers > 256 * 1024/*arbitrary*/)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad number of depth layers in scalar field file");
	}

	// Read the radius of each depth layer.
	// These assume the Earth has unit radius.
	// Deeper layers (closer to the Earth's core) have smaller radii.
	d_depth_layer_radii.reserve(d_num_depth_layers);
	for (unsigned int depth_layer_index = 0; depth_layer_index < d_num_depth_layers; ++depth_layer_index)
	{
		float depth_layer_radius;
		d_in >> depth_layer_radius;

		// Each layer radius should be in the range [0,1].
		// NOTE: No floating-point epsilon required in comparison since 0 and 1 represented exactly.
		if (depth_layer_radius < 0 || depth_layer_radius > 1)
		{
			throw FileFormatNotSupportedException(
					GPLATES_EXCEPTION_SOURCE, "bad depth layer radius in scalar field file");
		}

		d_depth_layer_radii.push_back(depth_layer_radius);
	}

	// Read the scalar minimum.
	double scalar_min;
	d_in >> scalar_min;
	d_scalar_min = scalar_min;

	// Read the scalar maximum.
	double scalar_max;
	d_in >> scalar_max;
	d_scalar_max = scalar_max;

	// Read the scalar mean.
	double scalar_mean;
	d_in >> scalar_mean;
	d_scalar_mean = scalar_mean;

	// Read the scalar standard deviation.
	double scalar_std_dev;
	d_in >> scalar_std_dev;
	d_scalar_standard_deviation = scalar_std_dev;

	// Read the gradient magnitude minimum.
	double gradient_magnitude_min;
	d_in >> gradient_magnitude_min;
	d_gradient_magnitude_min = gradient_magnitude_min;

	// Read the gradient magnitude maximum.
	double gradient_magnitude_max;
	d_in >> gradient_magnitude_max;
	d_gradient_magnitude_max = gradient_magnitude_max;

	// Read the gradient magnitude mean.
	double gradient_magnitude_mean;
	d_in >> gradient_magnitude_mean;
	d_gradient_magnitude_mean = gradient_magnitude_mean;

	// Read the gradient magnitude standard deviation.
	double gradient_magnitude_std_dev;
	d_in >> gradient_magnitude_std_dev;
	d_gradient_magnitude_standard_deviation = gradient_magnitude_std_dev;

	// Check that the file size is what we expect.
	const qint64 actual_data_size = d_file.size() - d_file.pos();
	const qint64 expected_data_size =
			6 * d_tile_meta_data_resolution * d_tile_meta_data_resolution * TileMetaData::STREAM_SIZE +
			d_num_active_tiles * d_num_depth_layers * d_tile_resolution * d_tile_resolution * FieldDataSample::STREAM_SIZE +
			d_num_active_tiles * d_tile_resolution * d_tile_resolution * MaskDataSample::STREAM_SIZE;
	if (actual_data_size != expected_data_size)
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "bad data size in scalar field file");
	}

	// The file offset to the tile metadata.
	d_tile_meta_data_file_offset = d_file.pos();

	// The file offset to the tile scalar value data (and gradient data).
	// Skip past the tile metadata.
	d_field_data_file_offset = d_tile_meta_data_file_offset +
			6 * d_tile_meta_data_resolution * d_tile_meta_data_resolution * TileMetaData::STREAM_SIZE;

	// The file offset to the tile scalar mask (validity) data.
	// Skip past the tile scalar value data.
	d_mask_data_file_offset = d_field_data_file_offset +
			d_num_active_tiles * d_num_depth_layers * d_tile_resolution * d_tile_resolution * FieldDataSample::STREAM_SIZE;
}


boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::TileMetaData>
GPlatesFileIO::ScalarField3DFileFormat::Reader::VersionOneReader::read_tile_meta_data() const
{
	// Seek to the metadata.
	if (!d_file.seek(d_tile_meta_data_file_offset))
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "error seeking to metadata in scalar field file");
	}

	const unsigned int num_tile_meta_datas = 6 * d_tile_meta_data_resolution * d_tile_meta_data_resolution;

	boost::shared_array<TileMetaData> tile_meta_data_array(new TileMetaData[num_tile_meta_datas]);

	for (unsigned int n = 0; n < num_tile_meta_datas; ++n)
	{
		TileMetaData &tile_meta_data = tile_meta_data_array[n];

		// Read as single-precision floating-point.
		float tile_ID, max_scalar_value, min_scalar_value;
		// Note that QDataStream::operator>> does endian-conversion for us.
		d_in >> tile_ID >> max_scalar_value >> min_scalar_value;

		tile_meta_data.tile_ID = tile_ID;
		tile_meta_data.max_scalar_value = max_scalar_value;
		tile_meta_data.min_scalar_value = min_scalar_value;

		// The tile ID should be in the range [0, d_num_active_tiles-1] - or it can be -1.
		// Which is an error if 'tile_ID >= d_num_active_tiles' but since storing integers as floats
		// we avoid compiler warning by re-writing as 'tile_ID > d_num_active_tiles - 1'
		// since compiler (rightly) warnings on comparing floats for equality - however we're using
		// floats to represents *integers* (exactly).
		if (tile_ID > d_num_active_tiles - 1 ||
			tile_ID < -1)
		{
			throw FileFormatNotSupportedException(
					GPLATES_EXCEPTION_SOURCE, "tile ID out-of-range in scalar field file");
		}
	}

	return tile_meta_data_array;
}



boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::FieldDataSample>
GPlatesFileIO::ScalarField3DFileFormat::Reader::VersionOneReader::read_field_data(
		unsigned int layer_index,
		unsigned int num_layers_to_read) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			layer_index + num_layers_to_read <= d_num_active_tiles * d_num_depth_layers,
			GPLATES_ASSERTION_SOURCE);

	// Seek to the field data.
	const qint64 file_offset = d_field_data_file_offset +
			layer_index * d_tile_resolution * d_tile_resolution * FieldDataSample::STREAM_SIZE;
	if (!d_file.seek(file_offset))
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "error seeking to tile field data in scalar field file");
	}

	const unsigned int num_field_data_samples = num_layers_to_read * d_tile_resolution * d_tile_resolution;

	boost::shared_array<FieldDataSample> field_data_array(new FieldDataSample[num_field_data_samples]);

	// NOTE: Since we're reading a lot of data we'll bypass the expensive output operator '>>' and
	// hence is *much* faster than doing a loop with '>>' (as determined by profiling).
	// We have to do our own endian conversion though.
	void *const raw_data = field_data_array.get();
	const unsigned int bytes_read = d_in.readRawData(
			static_cast<char *>(raw_data),
			num_field_data_samples * FieldDataSample::STREAM_SIZE);

	// Since we're reading directly into the structure as bytes we need to ensure the FieldDataSample
	// structure does not have any holes in it due to data member alignment issues.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			sizeof(FieldDataSample) == FieldDataSample::STREAM_SIZE,
			GPLATES_ASSERTION_SOURCE);

	if (bytes_read != num_field_data_samples * FieldDataSample::STREAM_SIZE)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Error reading tile field data from scalar field file.");
	}

	// Convert from the endian stored in the file to endian of the runtime system (if necessary).
	GPlatesUtils::Endian::convert(
			field_data_array.get(),
			field_data_array.get() + num_field_data_samples,
			static_cast<QSysInfo::Endian>(Q_DATA_STREAM_BYTE_ORDER));

	return field_data_array;
}


boost::shared_array<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample>
GPlatesFileIO::ScalarField3DFileFormat::Reader::VersionOneReader::read_mask_data(
		unsigned int tile_index,
		unsigned int num_tiles_to_read) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_index + num_tiles_to_read <= d_num_active_tiles,
			GPLATES_ASSERTION_SOURCE);

	// Seek to the mask data.
	const qint64 file_offset = d_mask_data_file_offset +
			tile_index * d_tile_resolution * d_tile_resolution * MaskDataSample::STREAM_SIZE;
	if (!d_file.seek(file_offset))
	{
		throw FileFormatNotSupportedException(
				GPLATES_EXCEPTION_SOURCE, "error seeking to tile mask data in scalar field file");
	}

	const unsigned int num_mask_data_samples = num_tiles_to_read * d_tile_resolution * d_tile_resolution;

	boost::shared_array<MaskDataSample> mask_data_array(new MaskDataSample[num_mask_data_samples]);

	// NOTE: Since we're reading a lot of data we'll bypass the expensive output operator '>>' and
	// hence is *much* faster than doing a loop with '>>' (as determined by profiling).
	// We have to do our own endian conversion though.
	void *const raw_data = mask_data_array.get();
	const unsigned int bytes_read = d_in.readRawData(
			static_cast<char *>(raw_data),
			num_mask_data_samples * MaskDataSample::STREAM_SIZE);

	// Since we're reading directly into the structure as bytes we need to ensure the MaskDataSample
	// structure does not have any holes in it due to data member alignment issues.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			sizeof(MaskDataSample) == MaskDataSample::STREAM_SIZE,
			GPLATES_ASSERTION_SOURCE);

	if (bytes_read != num_mask_data_samples * MaskDataSample::STREAM_SIZE)
	{
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Error reading tile mask data from scalar field file.");
	}

	// Convert from the endian stored in the file to endian of the runtime system (if necessary).
	GPlatesUtils::Endian::convert(
			mask_data_array.get(),
			mask_data_array.get() + num_mask_data_samples,
			static_cast<QSysInfo::Endian>(Q_DATA_STREAM_BYTE_ORDER));

	return mask_data_array;
}
