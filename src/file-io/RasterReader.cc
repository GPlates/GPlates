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

#include "opengl/OpenGLBadAllocException.h"
#include "opengl/OpenGLException.h"

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


	std::map<QString, RasterReader::FormatInfo>
	create_supported_formats_map()
	{
		using namespace GPlatesUtils::OverloadResolution;

		typedef std::map<QString, RasterReader::FormatInfo> formats_map_type;
		formats_map_type formats;

		// Add formats that we support via the RGBA reader.
		// Note: The descriptions are those used by the GIMP.
		formats.insert(std::make_pair(
				"bmp",
				RasterReader::FormatInfo(
					"Windows BMP image",
					"image/bmp",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"gif",
				RasterReader::FormatInfo(
					"GIF image",
					"image/gif",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"jpg",
				RasterReader::FormatInfo(
					"JPEG image",
					"image/jpeg",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"jpeg",
				RasterReader::FormatInfo(
					"JPEG image",
					"image/jpeg",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"png",
				RasterReader::FormatInfo(
					"PNG image",
					"image/png",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"svg",
				RasterReader::FormatInfo(
					"SVG image",
					"image/svg+xml",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"tif",
				RasterReader::FormatInfo(
					"TIFF image",
					"image/tiff",
					RasterReader::RGBA)));
		formats.insert(std::make_pair(
				"tiff",
				RasterReader::FormatInfo(
					"TIFF image",
					"image/tiff",
					RasterReader::RGBA)));

		// Also add GMT grid/NetCDF files, read by GDAL.
		formats.insert(std::make_pair(
				"grd",
				RasterReader::FormatInfo(
					"NetCDF/GMT grid data",
					"application/x-netcdf",
					RasterReader::GDAL)));
		formats.insert(std::make_pair(
				"nc",
				RasterReader::FormatInfo(
					"NetCDF/GMT grid data",
					"application/x-netcdf",
					RasterReader::GDAL)));

		return formats;
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


const std::map<QString, GPlatesFileIO::RasterReader::FormatInfo> &
GPlatesFileIO::RasterReader::get_supported_formats()
{
	static const std::map<QString, FormatInfo> supported_formats = create_supported_formats_map();
	return supported_formats;
}


const QString &
GPlatesFileIO::RasterReader::get_file_dialog_filters()
{
	static const QString file_dialog_filters = create_file_dialog_filters_string(get_supported_formats());
	return file_dialog_filters;
}


GPlatesFileIO::RasterReader::non_null_ptr_type
GPlatesFileIO::RasterReader::create(
		const QString &filename,
		ReadErrorAccumulation *read_errors)
{
	return new RasterReader(filename, read_errors);
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
	FormatHandler handler =
		(iter != supported_formats.end()) ? iter->second.handler : UNKNOWN;

	boost::function<RasterBandReaderHandle (unsigned int)> proxy_handle_function =
		boost::bind(&RasterReader::create_raster_band_reader_handle, boost::ref(*this), _1);

	switch (handler)
	{
		case RGBA:
			d_impl.reset(new RgbaRasterReader(filename, proxy_handle_function, read_errors));
			break;
		
		case GDAL:
			d_impl.reset(new GDALRasterReader(filename, proxy_handle_function, read_errors));
			break;

		default:
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


void *
GPlatesFileIO::RasterReader::get_data(
		unsigned int band_number,
		const QRect &region,
		ReadErrorAccumulation *read_errors)
{
	if (d_impl)
	{
		return d_impl->get_data(band_number, region, read_errors);
	}
	else
	{
		return NULL;
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

