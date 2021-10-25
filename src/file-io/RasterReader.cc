/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

#include "RasterReader.h"

#include "ErrorOpeningFileForReadingException.h"
#include "GdalRasterReader.h"
#include "RgbaRasterReader.h"
#include "ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"

#include "opengl/OpenGLBadAllocException.h"
#include "opengl/OpenGLException.h"

#include "property-values/ProxiedRasterResolver.h"

#include "utils/OverloadResolution.h"

using namespace GPlatesFileIO;

namespace
{
	/**
	 * Returns true if the filename is of the required form (i.e. <root>-<time>.grd), and sets the 
	 * value of time.
	 *
	 * Returns false if the filename is not of the required form.
	 */
	bool
	parse_filename(
			const QString filename, 
			QString &root,
			int &time)
	{
		// Strip extension from filename.
		
		QString filename_stripped = filename.section('.', -2, -2);

		// Split into <root> and <time> parts.
		QStringList parts = filename_stripped.split(QString("-"));	
		if (parts.size() != 2)
		{
			qDebug() << "Filename is not of required form.";
			return false;
		}

		QString potential_root = parts.at(0);
		QString num_string = parts.at(1);
		bool ok;
		time = num_string.toInt(&ok);
		if (!ok)
		{
			qDebug() << "Second field cannot be converted to an integer.";
			return false;
		}
		
		root = potential_root;
		return true;
	}


	template<class InputIterator, class OutputIterator, class UnaryOperator, class Predicate>
	OutputIterator
	transform_if(
			InputIterator begin,
			InputIterator end,
			OutputIterator result,
			UnaryOperator op,
			Predicate pred)
	{
		while (begin != end)
		{
			if (pred(*begin))
			{
				*result = op(*begin);
			}
			++result;
			++begin;
		}
		return result;
	}


	void
	add_supported_formats(
			std::map<QString, RasterReader::FormatInfo> &formats,
			RasterReader::FormatHandler format_handler)
	{
		using namespace GPlatesUtils::OverloadResolution;

		// Add formats that we support via the RGBA reader.
		// Note: The descriptions are those used by the GIMP.
		if (format_handler == RasterReader::RGBA)
		{
			formats.insert(std::make_pair(
					"bmp",
					RasterReader::FormatInfo(
						"Windows BMP",
						"image/bmp",
						RasterReader::RGBA)));
			formats.insert(std::make_pair(
					"gif",
					RasterReader::FormatInfo(
						"GIF",
						"image/gif",
						RasterReader::RGBA)));
			formats.insert(std::make_pair(
					"jpg",
					RasterReader::FormatInfo(
						"JPEG",
						"image/jpeg",
						RasterReader::RGBA)));
			formats.insert(std::make_pair(
					"jpeg",
					RasterReader::FormatInfo(
						"JPEG",
						"image/jpeg",
						RasterReader::RGBA)));
			formats.insert(std::make_pair(
					"png",
					RasterReader::FormatInfo(
						"PNG",
						"image/png",
						RasterReader::RGBA)));
			formats.insert(std::make_pair(
					"svg",
					RasterReader::FormatInfo(
						"SVG",
						"image/svg+xml",
						RasterReader::RGBA)));
		}

		// Also add support for numerical rasters (eg, GMT grid/NetCDF files), read by GDAL.
		// These formats can also support RGBA data such as GeoTIFF (*.tif) but have the advantage
		// (due to GDAL) of also supporting georeferencing and spatial reference systems
		// (unlike the RGBA raster reader above).
		if (format_handler == RasterReader::GDAL)
		{
			formats.insert(std::make_pair(
					"grd",
					RasterReader::FormatInfo(
						"NetCDF/GMT",
						"application/x-netcdf",
						RasterReader::GDAL)));
			formats.insert(std::make_pair(
					"nc",
					RasterReader::FormatInfo(
						"NetCDF/GMT",
						"application/x-netcdf",
						RasterReader::GDAL)));
			formats.insert(std::make_pair(
					"tif",
					RasterReader::FormatInfo(
						"TIFF",
						"image/tiff",
						RasterReader::GDAL)));
			formats.insert(std::make_pair(
					"tiff",
					RasterReader::FormatInfo(
						"TIFF",
						"image/tiff",
						RasterReader::GDAL)));
			formats.insert(std::make_pair(
					"img",
					RasterReader::FormatInfo(
						"Erdas Imagine",
						"application/x-erdas-hfa",
						RasterReader::GDAL)));
			formats.insert(std::make_pair(
					"ers",
					RasterReader::FormatInfo(
						"ERMapper",
						"application/x-ers",
						RasterReader::GDAL)));
		}
	}


	/**
	 * Creates a single entry in the filters string.
	 */
	QString
	create_file_dialog_filter_string(
			const QString &description,
			QStringList exts)
	{
		// Prepend *. to each extension first.
		using namespace GPlatesUtils::OverloadResolution;
		std::for_each(
				exts.begin(),
				exts.end(),
				boost::lambda::bind(
					resolve<mem_fn_types<QString>::prepend1>(&QString::prepend),
					boost::lambda::_1,
					"*."));

		return description + " (" + exts.join(" ") + ")";
	}


	class FormatAccumulator
	{
	public:
		FormatAccumulator(
				std::map<QString, QStringList> &descriptions_to_ext) :
			d_descriptions_to_ext(descriptions_to_ext)
		{
		}

		void operator()(
				const std::pair<const QString, RasterReader::FormatInfo> &format)
		{
			d_descriptions_to_ext[format.second.description].push_back(format.first);
		}

	private:
		std::map<QString, QStringList> &d_descriptions_to_ext;
	};


	QString
	create_file_dialog_filters_string(
			const std::map<QString, RasterReader::FormatInfo> &formats)
	{
		QStringList filters;

		// The first filter is an all-inclusive filter that matches all supported
		// raster formats.
		QStringList all_exts;
		std::for_each(
				formats.begin(),
				formats.end(),
				boost::ref(all_exts) <<
					boost::lambda::bind(
						&std::pair<const QString, RasterReader::FormatInfo>::first,
						boost::lambda::_1));
		filters << create_file_dialog_filter_string("All rasters", all_exts);

		// We then map textual descriptions to file extensions.
		// Note: jpg and jpeg (amongst others) have the same textual descriptions.
		std::map<QString, QStringList> descriptions_to_ext;
		std::for_each(
				formats.begin(),
				formats.end(),
				FormatAccumulator(descriptions_to_ext));

		// We then create one filter entry for each textual description.
		std::for_each(
				descriptions_to_ext.begin(),
				descriptions_to_ext.end(),
				boost::ref(filters) <<
					boost::lambda::bind(
						&create_file_dialog_filter_string,
						boost::lambda::bind(
							&std::pair<const QString, QStringList>::first,
							boost::lambda::_1),
						boost::lambda::bind(
							&std::pair<const QString, QStringList>::second,
							boost::lambda::_1)));

		// The last filter matches all files, regardless of file extension.
		QStringList all_files(QString("*"));
		filters << create_file_dialog_filter_string("All files", all_files);

		return filters.join(";;");
	}

}  // anonymous namespace


std::map<QString, GPlatesFileIO::RasterReader::FormatInfo>
GPlatesFileIO::RasterReader::get_supported_formats()
{
	std::map<QString, FormatInfo> supported_formats;

	for (unsigned int format_handler = 0; format_handler < NUM_FORMAT_HANDLERS; ++format_handler)
	{
		add_supported_formats(supported_formats, static_cast<FormatHandler>(format_handler));
	}

	return supported_formats;
}


std::map<QString, GPlatesFileIO::RasterReader::FormatInfo>
GPlatesFileIO::RasterReader::get_supported_formats(
		FormatHandler format_handler)
{
	std::map<QString, FormatInfo> supported_formats;
	add_supported_formats(supported_formats, format_handler);
	return supported_formats;
}


QString
GPlatesFileIO::RasterReader::get_file_dialog_filters()
{
	static const QString file_dialog_filters = create_file_dialog_filters_string(get_supported_formats());
	return file_dialog_filters;
}


QString
GPlatesFileIO::RasterReader::get_file_dialog_filters(
		FormatHandler format_handler)
{
	return create_file_dialog_filters_string(get_supported_formats(format_handler));
}


GPlatesFileIO::RasterReader::non_null_ptr_type
GPlatesFileIO::RasterReader::create(
		const QString &filename,
		ReadErrorAccumulation *read_errors)
{
	RasterReader::non_null_ptr_type raster_reader(new RasterReader(filename, read_errors));

	// By creating the raster reader we've ensured the source raster file cache has been created and
	// is up to date so do the same with the raster mipmaps file cache.
	// This way the slow creation of caches can be done up front during the GPML file loading phase
	// ensuring no hickups or delays during rendering (eg, if the user the mipmaps suddenly need
	// to be rendered they won't suffer a delay while the mipmaps file cache is built).
	for (unsigned int band_number = 1; band_number <= raster_reader->get_number_of_bands(); ++band_number)
	{
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raw_raster =
				raster_reader->get_proxied_raw_raster(band_number, read_errors);
		if (proxied_raw_raster)
		{
			boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxied_raster_resolver =
					GPlatesPropertyValues::ProxiedRasterResolver::create(proxied_raw_raster.get());
			if (proxied_raster_resolver)
			{
				proxied_raster_resolver.get()->ensure_mipmaps_available();
			}
		}
	}

	return raster_reader;
}


GPlatesFileIO::RasterReader::RasterReader(
		const QString &filename,
		ReadErrorAccumulation *read_errors) :
	d_impl(NULL),
	d_filename(filename)
{
	QFileInfo file_info(filename);
	QString suffix = file_info.suffix().toLower();

	const std::map<QString, FormatInfo> &supported_formats = get_supported_formats();
	std::map<QString, FormatInfo>::const_iterator iter = supported_formats.find(suffix);

	// If a supported format was not found...
	if (iter == supported_formats.end())
	{
		if (read_errors)
		{
			read_errors->d_failures_to_begin.push_back(
					make_read_error_occurrence(
						filename,
						DataFormats::RasterImage,
						0,
						ReadErrors::UnrecognisedRasterFileType,
						ReadErrors::FileNotLoaded));
		}
		return;
	}

	const FormatHandler handler = iter->second.handler;

	switch (handler)
	{
		case RGBA:
			d_impl.reset(new RgbaRasterReader(filename, this, read_errors));
			break;
		
		case GDAL:
			d_impl.reset(new GDALRasterReader(filename, this, read_errors));
			break;

		default:
			// Shouldn't be able to get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


const QString &
GPlatesFileIO::RasterReader::get_filename() const
{
	return d_filename;
}


bool
GPlatesFileIO::RasterReader::can_read()
{
	if (d_impl)
	{
		return d_impl->can_read();
	}
	else
	{
		return false;
	}
}


boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type>
GPlatesFileIO::RasterReader::get_georeferencing()
{
	if (d_impl)
	{
		return d_impl->get_georeferencing();
	}
	else
	{
		return boost::none;
	}
}


boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
GPlatesFileIO::RasterReader::get_spatial_reference_system()
{
	if (d_impl)
	{
		return d_impl->get_spatial_reference_system();
	}
	else
	{
		return boost::none;
	}
}


unsigned int
GPlatesFileIO::RasterReader::get_number_of_bands(
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_number_of_bands(read_errors);
	}
	else
	{
		return 0;
	}
}


std::pair<unsigned int, unsigned int>
GPlatesFileIO::RasterReader::get_size(
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_size(read_errors);
	}
	else
	{
		return std::make_pair(0, 0);
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterReader::get_proxied_raw_raster(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_proxied_raw_raster(band_number, read_errors);
	}
	else
	{
		return boost::none;
	}
}


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterReader::get_raw_raster(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_raw_raster(band_number, region, read_errors);
	}
	else
	{
		return boost::none;
	}
}


GPlatesPropertyValues::RasterType::Type
GPlatesFileIO::RasterReader::get_type(
		unsigned int band_number,
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_type(band_number, read_errors);
	}
	else
	{
		return GPlatesPropertyValues::RasterType::UNKNOWN;
	}
}


GPlatesFileIO::RasterBandReaderHandle
GPlatesFileIO::RasterReader::create_raster_band_reader_handle(
		unsigned int band_number)
{
	return RasterBandReaderHandle(
			RasterBandReader(non_null_ptr_type(this), band_number));
}


GPlatesFileIO::RasterReaderImpl::~RasterReaderImpl()
{  }

