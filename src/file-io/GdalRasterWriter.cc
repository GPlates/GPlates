/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <utility> // std::pair
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/ref.hpp>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <ogr_spatialref.h>

#include "GdalRasterWriter.h"

#include "Gdal.h"
#include "GdalUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/Georeferencing.h"
#include "property-values/RasterType.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/SpatialReferenceSystem.h"


namespace
{
	GDALDataType
	get_gdal_type_from_raster_type(
			GPlatesPropertyValues::RasterType::Type raster_type)
	{
		switch (raster_type)
		{
			case GPlatesPropertyValues::RasterType::UINT8:
				return GDT_Byte;

			case GPlatesPropertyValues::RasterType::UINT16:
				return GDT_UInt16;

			case GPlatesPropertyValues::RasterType::INT16:
				return GDT_Int16;

			case GPlatesPropertyValues::RasterType::UINT32:
				return GDT_UInt32;

			case GPlatesPropertyValues::RasterType::INT32:
				return GDT_Int32;

			case GPlatesPropertyValues::RasterType::FLOAT:
				return GDT_Float32;

			case GPlatesPropertyValues::RasterType::DOUBLE:
				return GDT_Float64;

			default:
				return GDT_Unknown;
		}
	}


	/**
	 * Returns true if the driver can create files (supports 'CREATECOPY').
	 */
	bool
	does_driver_support_creation(
			const char *driver_name)
	{
		GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driver_name);
		if (driver == NULL)
		{
			// Shouldn't be able to get here since driver name should be recognised.
			qWarning() << "Unable to get GDAL driver '" << driver_name << "' for querying file creation support.";
			return false;
		}

		char **driver_metadata = driver->GetMetadata();
		if (driver_metadata == NULL)
		{
			qWarning() << "Unable to get metadata for GDAL raster driver '" << driver_name << "'.";
			return false;
		}

		// Ignore drivers that cannot create a copy from another (in-memory) GDALDataset.
		if (!CSLFetchBoolean(driver_metadata, GDAL_DCAP_CREATECOPY, FALSE))
		{
			return false;
		}

		return true;
	}


	/**
	 * Determines which GDAL data (band) types are supported by the specified GDAL driver.
	 */
	std::vector<GPlatesPropertyValues::RasterType::Type>
	get_supported_band_types(
			const char *driver_name)
	{
		std::vector<GPlatesPropertyValues::RasterType::Type> supported_band_types;

		GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driver_name);
		if (driver == NULL)
		{
			// Shouldn't be able to get here since driver name should be recognised.
			qWarning() << "Unable to get GDAL driver '" << driver_name << "' for finding supported band types.";
			return supported_band_types;
		}

		char **driver_metadata = driver->GetMetadata();
		if (driver_metadata == NULL)
		{
			qWarning() << "Unable to get metadata for GDAL raster driver '" << driver_name << "'.";
			return supported_band_types;
		}

		// Ignore drivers that cannot create a copy from another (in-memory) GDALDataset.
		if (!CSLFetchBoolean(driver_metadata, GDAL_DCAP_CREATECOPY, FALSE))
		{
			// Shouldn't be able to get here since list of populated drivers should support 'CREATECOPY'.
			qWarning() << "GDAL raster driver '" << driver_name << "' does not support creating rasters.";
			return supported_band_types;
		}

		const char *creation_data_types = CSLFetchNameValue(driver_metadata, GDAL_DMD_CREATIONDATATYPES);
		if (!creation_data_types)
		{
			if (QString(driver_name) == "netCDF")
			{
				// The 'netCDF' driver does not have the 'GDAL_DMD_CREATIONDATATYPES' metadata so
				// just use the 'GMT' driver instead since it has this metadata and both drivers
				// support the same data types.
				return get_supported_band_types("GMT");
			}
			else
			{
				qWarning() << "GDAL raster driver '" << driver_name << "' does not support any band types for writing.";
				return supported_band_types;
			}
		}

		// Extract the space-separated data types from the string.
		const QStringList creation_data_types_list = QString(creation_data_types)
				.split(' ', QString::SkipEmptyParts, Qt::CaseInsensitive);

		// Convert each data type to a band type.
		for (int n = 0; n < creation_data_types_list.size(); ++n)
		{
			const QString creation_data_type = creation_data_types_list[n];

			if (creation_data_type == "Byte")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::UINT8);
				// A colour raster is made up of multiple 'Byte' channel bands.
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::RGBA8);
			}
			else if (creation_data_type == "UInt16")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::UINT16);
			}
			else if (creation_data_type == "Int16")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::INT16);
			}
			else if (creation_data_type == "UInt32")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::UINT32);
			}
			else if (creation_data_type == "Int32")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::INT32);
			}
			else if (creation_data_type == "Float32")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::FLOAT);
			}
			else if (creation_data_type == "Float64")
			{
				supported_band_types.push_back(GPlatesPropertyValues::RasterType::DOUBLE);
			}
		}

#if 0
		qDebug() << "Driver name '" << driver_name << "':";
		for (unsigned int n = 0; n < supported_band_types.size(); ++n)
		{
			qDebug() << "  " << GPlatesPropertyValues::RasterType::get_type_as_string(supported_band_types[n]);
		}
#endif

		return supported_band_types;
	}
}


GPlatesFileIO::GDALRasterWriter::format_desc_to_internal_format_info_map_type
		GPlatesFileIO::GDALRasterWriter::s_format_desc_to_internal_format_info_map;


void
GPlatesFileIO::GDALRasterWriter::get_supported_formats(
		RasterWriter::supported_formats_type &supported_formats)
{
	// Ensure all drivers have been registered.
	GdalUtils::gdal_register_drivers();

	// Add support for numerical rasters (eg, GMT grid/NetCDF files), written by GDAL.
	// These formats can also support RGBA data such as GeoTIFF (*.tif) but have the advantage
	// (due to GDAL) of also supporting georeferencing and spatial reference systems
	// (unlike the RGBA raster writer above).

	add_supported_format(
			supported_formats,
			"nc", // filename_extension
			"NetCDF grid data", // format_description
			"application/x-netcdf", // format_mime_type
			"netCDF"); // driver_name

	add_supported_format(
			supported_formats,
			"grd", // filename_extension
			"GMT grid data", // format_description
			"application/x-netcdf", // format_mime_type
			"GMT"); // driver_name

	add_supported_format(
			supported_formats,
			"tif", // filename_extension
			"TIFF image", // format_description
			"image/tiff", // format_mime_type
			"GTiff"); // driver_name

	add_supported_format(
			supported_formats,
			"tiff", // filename_extension
			"TIFF image", // format_description
			"image/tiff", // format_mime_type
			"GTiff"); // driver_name

	// HFA driver does not export statistics by default.
	std::vector<std::string> hfa_creation_options;
	hfa_creation_options.push_back("STATISTICS=YES");
	add_supported_format(
			supported_formats,
			"img", // filename_extension
			"Erdas Imagine", // format_description
			"application/x-erdas-hfa", // format_mime_type
			"HFA", // driver_name
			hfa_creation_options);

	add_supported_format(
			supported_formats,
			"ers", // filename_extension
			"ERMapper", // format_description
			"application/x-ers", // format_mime_type
			"ERS"); // driver_name
}


void
GPlatesFileIO::GDALRasterWriter::add_supported_format(
		GPlatesFileIO::RasterWriter::supported_formats_type &supported_formats,
		const QString &filename_extension,
		const QString &format_description,
		const QString &format_mime_type,
		const char *driver_name,
		const std::vector<std::string> &creation_options)
{
	// Return early if the driver does not support creation.
	if (!does_driver_support_creation(driver_name))
	{
		return;
	}

	// Return early if driver does not support any of our band types.
	const std::vector<GPlatesPropertyValues::RasterType::Type> supported_band_types =
			get_supported_band_types(driver_name);
	if (supported_band_types.empty())
	{
		return;
	}

	// Insert the supported format entries into the map of supported formats and also
	// map the format descriptions to GDAL driver names to use ourselves later.

	supported_formats.insert(
			std::make_pair(
					filename_extension,
					GPlatesFileIO::RasterWriter::FormatInfo(
							format_description,
							format_mime_type,
							GPlatesFileIO::RasterWriter::GDAL,
							supported_band_types)));

	s_format_desc_to_internal_format_info_map.insert(
			std::make_pair(
					format_description,
					InternalFormatInfo(driver_name, creation_options)));
}


const GPlatesFileIO::GDALRasterWriter::InternalFormatInfo &
GPlatesFileIO::GDALRasterWriter::get_internal_format_info(
		const GPlatesFileIO::RasterWriter::FormatInfo &format_info)
{
	format_desc_to_internal_format_info_map_type::const_iterator iter =
			s_format_desc_to_internal_format_info_map.find(format_info.description);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			iter != s_format_desc_to_internal_format_info_map.end(),
			GPLATES_ASSERTION_SOURCE);

	return iter->second;
}


GPlatesFileIO::GDALRasterWriter::GDALRasterWriter(
		const QString &filename,
		const RasterWriter::FormatInfo &format_info,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int num_raster_bands,
		GPlatesPropertyValues::RasterType::Type raster_band_type) :
	d_filename(filename),
	d_num_raster_bands(num_raster_bands),
	d_raster_band_type(raster_band_type),
	d_internal_format_info(get_internal_format_info(format_info)),
	// A NULL value results in 'can_write()' returning false.
	d_in_memory_dataset(NULL),
	d_file_driver(NULL)
{
	// Ensure all drivers have been registered.
	GdalUtils::gdal_register_drivers();

	// Make sure the driver that will be used to write the filename supports 'CREATECOPY' so we
	// can copy from our in-memory dataset to the file format's dataset.
	const std::string file_driver_name = d_internal_format_info.driver_name;
	d_file_driver = GetGDALDriverManager()->GetDriverByName(file_driver_name.c_str());
	if (d_file_driver == NULL)
	{
		// Shouldn't be able to get here since driver name should be recognised.
		qWarning() << "Unable to get GDAL driver '" << file_driver_name.c_str() << "' for writing raster '"
			<< d_filename << "'.";
		return;
	}
	char **file_driver_metadata = d_file_driver->GetMetadata();
	if (file_driver_metadata == NULL)
	{
		qWarning() << "Unable to get metadata for GDAL raster driver '" << file_driver_name.c_str() << "'.";
		return;
	}
	if (!CSLFetchBoolean(file_driver_metadata, GDAL_DCAP_CREATECOPY, FALSE))
	{
		// Shouldn't be able to get here since list of populated drivers should support 'CREATECOPY'.
		qWarning() << "GDAL raster driver '" << file_driver_name.c_str() << "' does not support creating rasters.";
		return;
	}

	// Create an in-memory dataset since it supports all GDALDataSet capabilities.
	// When we write the file we will use 'CREATECOPY' (sequential write from in-memory dataset)
	// since it's supported by a lot more drivers than 'Create' (random write).
	GDALDriver *in_memory_driver = GetGDALDriverManager()->GetDriverByName("MEM");
	if (!in_memory_driver)
	{
		qWarning() << "Unable to get GDAL driver (in-memory dataset) for writing rasters.";
		return;
	}

	// The filename does not matter for the in-memory dataset.
	const char *const in_memory_dataset_name = "in-memory";

	if (d_raster_band_type == GPlatesPropertyValues::RasterType::RGBA8)
	{
		// Can only have one *colour* band, in the RasterWriter API, which is made up of
		// four GDAL (channel) bands.
		if (d_num_raster_bands != 1)
		{
			qWarning() << "GDAL coloured rasters being written must be a single band.";
			return;
		}

		// Each GDAL colour band will have data type 'GDT_Byte'.
		d_in_memory_dataset =
				in_memory_driver->Create(
						in_memory_dataset_name,
						raster_width,
						raster_height,
						4/*GDAL colour channel bands*/,
						GDT_Byte,
						NULL);
		if (d_in_memory_dataset == NULL)
		{
			qWarning() << "Unable to create in-memory dataset for writing rasters.";
			return;
		}

		const GDALColorInterp gdal_colour_interp[4] = { GCI_RedBand, GCI_GreenBand, GCI_BlueBand, GCI_AlphaBand };

		// Set the colour interpretation of each GDAL band.
		for (unsigned int n = 1; n <= 4; ++n)
		{
			GDALRasterBand *raster_band = d_in_memory_dataset->GetRasterBand(n);
			if (raster_band == NULL)
			{
				// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
				GDALClose(d_in_memory_dataset);
				d_in_memory_dataset = NULL;
				qWarning() << "Unable to get in-memory raster band for writing rasters.";
				return;
			}

			// Each raster band represents a different channel of the RGBA raster.
			if (raster_band->SetColorInterpretation(gdal_colour_interp[n - 1]) != CE_None)
			{
				// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
				GDALClose(d_in_memory_dataset);
				d_in_memory_dataset = NULL;
				qWarning() << "Unable to set colour interpretation on in-memory raster band for writing rasters.";
				return;
			}
		}

		// Allocate space for the optional no-data value for the single band.
		// But there will never be a no-data value for colour (RGBA) rasters.
		d_raster_band_no_data_values.resize(1, boost::none);
	}
	else // One or more non-colour bands...
	{
		if (d_num_raster_bands == 0)
		{
			qWarning() << "GDAL rasters being written cannot have zero bands.";
			return;
		}

		// Exclude uninitialised and unknown raster band types.
		const GDALDataType gdal_data_type = get_gdal_type_from_raster_type(d_raster_band_type);
		if (gdal_data_type == GDT_Unknown)
		{
			qWarning() << "GDAL raster being written has unknown raster band type.";
			return;
		}

		d_in_memory_dataset =
				in_memory_driver->Create(
						in_memory_dataset_name,
						raster_width,
						raster_height,
						d_num_raster_bands,
						gdal_data_type,
						NULL);
		if (d_in_memory_dataset == NULL)
		{
			qWarning() << "Unable to create in-memory dataset for writing rasters.";
			return;
		}

		// Allocate space for the optional no-data value for each band.
		d_raster_band_no_data_values.resize(d_num_raster_bands, boost::none);
	}
}


GPlatesFileIO::GDALRasterWriter::~GDALRasterWriter()
{
	try
	{
		if (d_in_memory_dataset)
		{
			// Closes the dataset as well as all bands that were opened.
			GDALClose(d_in_memory_dataset);
		}
	}
	catch (...)
	{
	}
}


bool
GPlatesFileIO::GDALRasterWriter::can_write() const
{
	return d_in_memory_dataset != NULL;
}


void
GPlatesFileIO::GDALRasterWriter::set_georeferencing(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing)
{
	if (!can_write())
	{
		return;
	}

	double affine_geo_transform[6];

	// Extract the affine transform parameters.
	GPlatesPropertyValues::Georeferencing::parameters_type geo_parameters = georeferencing->parameters();
	for (unsigned int i = 0; i != GPlatesPropertyValues::Georeferencing::parameters_type::NUM_COMPONENTS; ++i)
	{
		affine_geo_transform[i] = geo_parameters.components[i];
	}

	if (d_in_memory_dataset->SetGeoTransform(affine_geo_transform) != CE_None)
	{
		// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
		GDALClose(d_in_memory_dataset);
		d_in_memory_dataset = NULL;
		qWarning() << "Unable to set geoferencing on GDAL raster '" << d_filename << "'.";
		return;
	}
}


void
GPlatesFileIO::GDALRasterWriter::set_spatial_reference_system(
		const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type& srs)
{
	if (!can_write())
	{
		return;
	}

	// GDALDataset expects the spatial reference system in WKT format.
    char *srcWKT = NULL;
	if (srs->get_ogr_srs().exportToWkt(&srcWKT) != OGRERR_NONE)
	{
		CPLFree(srcWKT);
		// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
		GDALClose(d_in_memory_dataset);
		d_in_memory_dataset = NULL;
		qWarning() << "Unable to extract WKT spatial reference system for GDAL raster '" << d_filename << "'.";
		return;
	}

	if (d_in_memory_dataset->SetProjection(srcWKT) != CE_None)
	{
		CPLFree(srcWKT);
		// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
		GDALClose(d_in_memory_dataset);
		d_in_memory_dataset = NULL;
		qWarning() << "Unable to set spatial reference system for GDAL raster '" << d_filename << "'.";
		return;
	}

	CPLFree(srcWKT);
}


bool
GPlatesFileIO::GDALRasterWriter::write_region_data(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &region_data,
		unsigned int band_number,
		unsigned int x_offset,
		unsigned int y_offset)
{
	if (!can_write())
	{
		return false;
	}

	// Band number should be in the valid range.
	if (band_number == 0 ||
		band_number > d_num_raster_bands)
	{
		qWarning() << "GDAL raster band number is outside valid range.";
		return false;
	}

	if (d_raster_band_type == GPlatesPropertyValues::RasterType::RGBA8)
	{
		return write_colour_region_data(*region_data, x_offset, y_offset);
	}
	// else one or more integer or floating-point bands...

	return write_numerical_region_data(*region_data, band_number, x_offset, y_offset);
}


bool
GPlatesFileIO::GDALRasterWriter::write_file()
{
	if (!can_write())
	{
		return false;
	}

	// Compute statistics on the in-memory dataset so that it will get copied out to the file.
	//
	// This appears to be needed by ArcGIS when loading Erdas Imagine rasters to avoid rasters
	// appearing black (until user explicitly re-calculates statistics in ArcGIS).
	const unsigned int num_gdal_bands = d_in_memory_dataset->GetRasterCount();
	for (unsigned int n = 0; n < num_gdal_bands; ++n)
	{
		GDALRasterBand *raster_band = d_in_memory_dataset->GetRasterBand(n + 1);
		if (raster_band)
		{
			// Compute the statistics for the current band - this also sets the statistics on the band.
			// If we get an error computing for one or more bands then they just won't have any statistics.
			raster_band->ComputeStatistics(false/*bApproxOK*/, NULL, NULL, NULL, NULL, NULL, NULL);
		}
	}

	const std::string filename_std_string = d_filename.toStdString();

	// Get the creation options ready to pass to GDAL.
	// Need some workarounds to avoid reinterpret casts apparently required for GDAL's lack of 'const'.
	std::vector< std::vector<char> > creation_options;
	std::vector<char *> creation_option_pointers;
	char **gdal_creation_option_pointers = NULL;
	if (!d_internal_format_info.creation_options.empty())
	{
		BOOST_FOREACH(const std::string &creation_option, d_internal_format_info.creation_options)
		{
			std::vector<char> option(creation_option.begin(), creation_option.end());
			option.push_back('\0'); // Null terminate the string.
			creation_options.push_back(option);
		}
		BOOST_FOREACH(std::vector<char> &creation_option, creation_options)
		{
			creation_option_pointers.push_back(&creation_option[0]);
		}
		// Null terminate the list of strings.
		creation_option_pointers.push_back(NULL);

		gdal_creation_option_pointers = &creation_option_pointers[0];
	}

	// Copy the in-memory dataset to the file.
	GDALDataset *file_dataset = d_file_driver->CreateCopy(
			filename_std_string.c_str(),
			d_in_memory_dataset,
			FALSE,
			gdal_creation_option_pointers,
			NULL,
			NULL);
	if (file_dataset == NULL)
	{
		// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
		GDALClose(d_in_memory_dataset);
		d_in_memory_dataset = NULL;
		qWarning() << "Unable to create GDAL raster file '" << d_filename << "' from in-memory raster.";
		return false;
	}

	// Close file dataset to flush the written data to disk.
	GDALClose(file_dataset);
	file_dataset = NULL;

	// Close in-memory dataset and set to NULL - calls to 'can_write()' will then return false.
	GDALClose(d_in_memory_dataset);
	d_in_memory_dataset = NULL;

	return true;
}


bool
GPlatesFileIO::GDALRasterWriter::write_colour_region_data(
		GPlatesPropertyValues::RawRaster &region_data,
		unsigned int x_offset,
		unsigned int y_offset)
{
	// One band made up of four GDAL colour channel (RGBA) bands.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_raster_bands == 1 &&
				d_in_memory_dataset->GetRasterCount() == 4,
			GPLATES_ASSERTION_SOURCE);

	// The raster data must be RGBA.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> rgba8_region_data_opt =
			GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(region_data);
	if (!rgba8_region_data_opt)
	{
		qWarning() << "Expecting RGBA region data when writing to GDAL colour raster.";
		return false;
	}
	GPlatesPropertyValues::Rgba8RawRaster &rgba8_region_data = *rgba8_region_data_opt.get();

	const unsigned int region_width = rgba8_region_data.width();
	const unsigned int region_height = rgba8_region_data.height();

	// The raster data region being written must fit within the raster dimensions.
	if (x_offset + region_width > boost::numeric_cast<unsigned int>(d_in_memory_dataset->GetRasterXSize()) ||
		y_offset + region_height > boost::numeric_cast<unsigned int>(d_in_memory_dataset->GetRasterYSize()))
	{
		qWarning() << "Region written to GDAL raster is outside raster boundary.";
		return false;
	}

	// Get the colour RGBA bands.
	GDALRasterBand *raster_bands[4];
	for (unsigned int n = 0; n < 4; ++n)
	{
		raster_bands[n] = d_in_memory_dataset->GetRasterBand(n + 1);

		if (raster_bands[n] == NULL)
		{
			qWarning() << "Unable to get in-memory colour raster band for writing rasters.";
			return false;
		}
	}

	// Write the raster colour data out line by line.
	for (unsigned int i = 0; i != region_height; ++i)
	{
		// Destination write pointer.
		// Using qint64 in case reading file larger than 4Gb...
		GPlatesGui::rgba8_t *const rgba8_region_line_ptr = rgba8_region_data.data() + qint64(i) * region_width;
		boost::uint8_t *const rgba8_region_line_byte_ptr = reinterpret_cast<boost::uint8_t *>(rgba8_region_line_ptr);

		for (unsigned int n = 0; n < 4; ++n)
		{
			// Write the line from the region data to the raster band of the current colour channel.
			CPLErr error = raster_bands[n]->RasterIO(
					GF_Write,
					x_offset,
					y_offset + i,
					region_width,
					1 /* write one row */,
					// Using qint64 in case writing file larger than 4Gb...
					rgba8_region_line_byte_ptr + n/*channel offset*/,
					region_width,
					1 /* one row of buffer */,
					// Each GDAL colour band will have data type 'GDT_Byte'...
					GDT_Byte,
					sizeof(GPlatesGui::rgba8_t),
					0 /* no offsets in buffer */);

			if (error != CE_None)
			{
				qWarning() << "Unable to write region colour channel data to in-memory raster band.";
				return false;
			}
		}
	}

	return true;
}


bool
GPlatesFileIO::GDALRasterWriter::write_numerical_region_data(
		GPlatesPropertyValues::RawRaster &region_data,
		unsigned int band_number,
		unsigned int x_offset,
		unsigned int y_offset)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_raster_band_no_data_values.size() == d_num_raster_bands &&
				boost::numeric_cast<unsigned int>(d_in_memory_dataset->GetRasterCount())
					== d_num_raster_bands,
			GPLATES_ASSERTION_SOURCE);

	WriteNumericalRegionDataVisitor visitor(
			d_in_memory_dataset,
			band_number,
			d_raster_band_type,
			// Using boost::ref to transport 'boost::optional<double> &' from
			// WriteNumericalRegionDataVisitor to WriteNumericalRegionDataVisitorImpl
			// (due to the former's use of 'const' references)...
			boost::ref(d_raster_band_no_data_values[band_number - 1]),
			x_offset,
			y_offset);
	region_data.accept_visitor(visitor);

	return visitor.wrote_region();
}


template <class RawRasterType>
bool
GPlatesFileIO::GDALRasterWriter::WriteNumericalRegionDataVisitorImpl::write_numerical_region_data(
		RawRasterType &region_data)
{
	typedef typename RawRasterType::element_type region_element_type;

	GDALRasterBand *raster_band = d_in_memory_dataset->GetRasterBand(d_band_number);
	if (raster_band == NULL)
	{
		qWarning() << "Unable to get in-memory raster band for writing rasters.";
		return false;
	}

	const unsigned int region_width = region_data.width();
	const unsigned int region_height = region_data.height();

	// The raster data region being written must fit within the raster dimensions.
	if (d_x_offset + region_width > boost::numeric_cast<unsigned int>(raster_band->GetXSize()) ||
		d_y_offset + region_height > boost::numeric_cast<unsigned int>(raster_band->GetYSize()))
	{
		qWarning() << "Region written to GDAL raster is outside raster boundary.";
		return false;
	}

	const GPlatesPropertyValues::RasterType::Type region_raster_band_type =
			GPlatesPropertyValues::RasterType::get_type_as_enum<region_element_type>();
	const GDALDataType region_gdal_data_type = get_gdal_type_from_raster_type(region_raster_band_type);
	if (region_gdal_data_type == GDT_Unknown)
	{
		// The region data raster type is uninitialised or unknown.
		// We shouldn't be able to get here though.
		return false;
	}

	// Get the region raster's no-data value (if any).
	// The 'no_data_value()' method will always exist on the region raster type because we
	// have already filtered out other raw raster types.
	if (region_data.no_data_value())
	{
		if (d_band_no_data_value)
		{
			// We're not the first region (for the current band) to have a no-data value.
			// So we check that it's the same no-data value.
			// This is an integer-to-integer or floating-point-to-floating-point comparison.
			// The latter (floating-point) will look for a NaN which should always succeed since
			// NaN is the only possible no-data value for a floating-point raster.
			if (!region_data.is_no_data_value(static_cast<region_element_type>(d_band_no_data_value.get())))
			{
				qWarning() << "Regions written to GDAL raster have conflicting no-data values.";
				return false;
			}
		}
		else // First region data (for the current band) that has a no-data value...
		{
			// Write the no-data value to the band.
			d_band_no_data_value = static_cast<double>(region_data.no_data_value().get());
			if (raster_band->SetNoDataValue(d_band_no_data_value.get()) != CE_None)
			{
				qWarning() << "Unable to set no-data value on in-memory raster band when writing raster.";
				return false;
			}
		}
	}

	// Write the raw raster data out line by line.
	for (unsigned int i = 0; i != region_height; ++i)
	{
		// Write the line from the region data to the raster band.
		CPLErr error = raster_band->RasterIO(
				GF_Write,
				d_x_offset,
				d_y_offset + i,
				region_width,
				1 /* write one row */,
				// Using qint64 in case writing file larger than 4Gb...
				region_data.data() + qint64(i) * region_width,
				region_width,
				1 /* one row of buffer */,
				// GDAL will convert between source and target data types (eg, float <-> double)...
				region_gdal_data_type,
				0 /* no offsets in buffer */,
				0 /* no offsets in buffer */);

		if (error != CE_None)
		{
			qWarning() << "Unable to write region numerical data to in-memory raster band.";
			return false;
		}
	}

	return true;
}
