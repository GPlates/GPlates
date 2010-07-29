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
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <QDir>
#include <QImageReader>
#include <QStringList>
#include <QDebug>

#include "ErrorOpeningFileForReadingException.h"
#include "GdalReader.h"
#include "ReadErrorAccumulation.h"
#include "RasterReader.h"

#include "gui/OpenGLBadAllocException.h"
#include "gui/OpenGLException.h"
#include "gui/Texture.h"

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


#if 0
// TGA support is commented out because it's not used right now.

/*
 * We use this macro to compare the return value of 'fread' (the number of objects read) with the
 * number of objects expected, and return (after closing the file to avoid a resource leak) if the
 * two numbers don't match.
 *
 * We weren't initially making this comparison, but now G++ 4.4.1 complains if we don't:
 *   RasterReader.cc: In function 'void<unnamed>::load_tga_file(const QString&,
 *     GPlatesPropertyValues::InMemoryRaster&, GPlatesFileIO::ReadErrorAccumulation&)':
 *   RasterReader.cc:103: error: ignoring return value of 'size_t fread(void*, size_t, size_t,
 *     FILE*)', declared with attribute warn_unused_result
 *
 * The behaviour of returning without complaining was chosen because this is how the existing code
 * in this function was handling error conditions.
 *
 * FIXME:  We should report to the user that there was an error reading their file (and what the
 * problem was) rather than silently dropping the file.
 */
#define FREAD_OR_RETURN(obj_ptr, obj_size, num_objs, stream) \
		if (fread((obj_ptr), (obj_size), (num_objs), (stream)) != (num_objs)) { \
			fclose(stream); \
			return; \
		}


	void
	load_tga_file(
			const QString &filename,
			GPlatesPropertyValues::InMemoryRaster &raster,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		unsigned char temp_char;
		unsigned char type;
		unsigned short temp_short;
		unsigned short width, height;
		unsigned char bpp;

		FILE *tga_file;
		if ( ! (tga_file = fopen(filename.toStdString().c_str(), "rb"))) {
			return;
		}

		// skip over some header data
		FREAD_OR_RETURN(&temp_char, sizeof (unsigned char), 1, tga_file)
		FREAD_OR_RETURN(&temp_char, sizeof (unsigned char), 1, tga_file)

		// get the type, and make sure it's RGB
		FREAD_OR_RETURN(&type, sizeof (unsigned char), 1, tga_file)

		if (type != 2) {
			fclose(tga_file);
			return;
		}

		// skip over some more header data
		FREAD_OR_RETURN(&temp_short, sizeof(unsigned short), 1, tga_file)
		FREAD_OR_RETURN(&temp_short, sizeof(unsigned short), 1, tga_file)
		FREAD_OR_RETURN(&temp_char, sizeof(unsigned char), 1, tga_file)
		FREAD_OR_RETURN(&temp_short, sizeof(unsigned short), 1, tga_file)
		FREAD_OR_RETURN(&temp_short, sizeof(unsigned short), 1, tga_file)

		// read the width, height, and bits-per-pixel.
		FREAD_OR_RETURN(&width, sizeof(unsigned short), 1, tga_file)
		FREAD_OR_RETURN(&height, sizeof(unsigned short), 1, tga_file)
		FREAD_OR_RETURN(&bpp, sizeof(unsigned char), 1, tga_file)

		FREAD_OR_RETURN(&temp_char, sizeof(unsigned char), 1, tga_file)

		if (bpp != 24) {
			fclose(tga_file);
			return;
		}

		size_t size = width * height; 

		std::vector<GLubyte> image_data_vector(size*3);

		FREAD_OR_RETURN(&image_data_vector[0], sizeof(unsigned char), size*3, tga_file)

		fclose (tga_file);

		// change BGR to RGB
		GLubyte temp;
		for (size_t i = 0; i < size * 3; i += 3) {
			temp = image_data_vector[i];
			image_data_vector[i] = image_data_vector[i + 2];
			image_data_vector[i + 2] = temp;
		}

		GPlatesPropertyValues::InMemoryRaster::ColourFormat format =
				GPlatesPropertyValues::InMemoryRaster::RgbFormat;
		raster.set_corresponds_to_data(false);

		QSize image_size = QSize(width,height);

		raster.generate_raster(image_data_vector, image_size, format);
		raster.set_enabled(true);
	}
#endif


	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
	load_qimage_file(
			const QString &filename,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		// Read the image in using the QImageReader.
		QImageReader image_reader(filename);
		QImage raw_image = image_reader.read();
		if (raw_image.isNull())
		{
			read_errors.d_failures_to_begin.push_back(
					make_read_error_occurrence(
						filename,
						DataFormats::RasterImage,
						0,
						ReadErrors::ErrorReadingQImageFile,
						ReadErrors::FileNotLoaded));
			return false;
		}

		// Convert it to RGBA format.
		QImage image = QGLWidget::convertToGLFormat(raw_image);
		QSize image_size = image.size();

		// Copy it into a Rgba8RawRaster.
		GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type raw_raster =
			GPlatesPropertyValues::Rgba8RawRaster::create(
					image_size.width(), image_size.height());
		GPlatesGui::rgba8_t *raw_raster_buf = raw_raster->data();
		GPlatesGui::rgba8_t *source_buf = reinterpret_cast<GPlatesGui::rgba8_t *>(image.bits());
		std::copy(
				source_buf,
				source_buf + image_size.width() * image_size.height(),
				raw_raster_buf);

		return static_cast<GPlatesPropertyValues::RawRaster::non_null_ptr_type>(raw_raster);
	}


	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
	load_gdal_file(
			const QString &filename,
			GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		GPlatesFileIO::GdalReader reader;
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raw_raster_opt =
			reader.read_file(filename, read_errors);

		return raw_raster_opt;
	}


	void
	add_to_formats_map(
			std::map<QString, RasterReader::FormatInfo> &formats,
			const std::map<QString, QString> &descriptions,
			QString ext)
	{
		formats.insert(
				std::make_pair(
					ext,
					RasterReader::FormatInfo(
						descriptions.find(ext)->second,
						RasterReader::Q_IMAGE_READER)));
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

		// Build a map from file extension to human-readable description for formats
		// read by QImageReader.
		// ico and mng are left out because surely no-one sane would use those for rasters.
		// Note: The descriptions are those used by the GIMP.
		typedef std::map<QString, QString> descriptions_map_type;
		std::map<QString, QString> descriptions;
		descriptions.insert(std::make_pair("bmp", "Windows BMP image"));
		descriptions.insert(std::make_pair("gif", "GIF image"));
		descriptions.insert(std::make_pair("jpg", "JPEG image"));
		descriptions.insert(std::make_pair("jpeg", "JPEG image"));
		descriptions.insert(std::make_pair("pbm", "PNM image"));
		descriptions.insert(std::make_pair("pgm", "PNM image"));
		descriptions.insert(std::make_pair("png", "PNG image"));
		descriptions.insert(std::make_pair("ppm", "PNM image"));
		descriptions.insert(std::make_pair("svg", "SVG image"));
		descriptions.insert(std::make_pair("tif", "TIFF image"));
		descriptions.insert(std::make_pair("tiff", "TIFF image"));
		descriptions.insert(std::make_pair("xbm", "X BitMap image"));
		descriptions.insert(std::make_pair("xpm", "X PixMap image"));

		// Add the formats that will be read using QImageReader.
		QList<QByteArray> qt_formats = QImageReader::supportedImageFormats();

		transform_if(
				qt_formats.begin(),
				qt_formats.end(),
				std::inserter(formats, formats.begin()),

				// We insert into the formats map ...
				boost::lambda::bind(
					boost::lambda::constructor<std::pair<QString, RasterReader::FormatInfo> >(),
					boost::lambda::_1, // ... a key of the file extension ...
					boost::lambda::bind(
						boost::lambda::constructor<RasterReader::FormatInfo>(), // ... and a corresponding FormatInfo value.
						boost::lambda::bind(
							&descriptions_map_type::operator[],
							boost::ref(descriptions),
							boost::lambda::_1),
						RasterReader::Q_IMAGE_READER)),

				// We only expose those formats for which we have a textual description.
				boost::lambda::bind(
					resolve<mem_fn_types<descriptions_map_type>::const_find>(&descriptions_map_type::find),
					boost::cref(descriptions),
					boost::lambda::_1) != descriptions.end());

		// Also add GMT grid files, read by GDAL.

		formats.insert(
				std::make_pair(
					"grd",
					RasterReader::FormatInfo(
						"GMT grid image",
						RasterReader::GDAL)));

#if 0
		for (std::map<QString, RasterReader::FormatInfo>::iterator iter = formats.begin();
				iter != formats.end(); ++iter)
		{
			qDebug() << iter->first << iter->second.description;
		}
#endif

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


boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
GPlatesFileIO::RasterReader::read_file(
	   const FileInfo &file_info,
	   ReadErrorAccumulation &read_errors)
{
	QFileInfo q_file_info = file_info.get_qfileinfo();

	QString suffix = q_file_info.suffix().toLower();
	QString file_path = file_info.get_qfileinfo().absoluteFilePath();

	const std::map<QString, FormatInfo> &supported_formats = get_supported_formats();
	std::map<QString, FormatInfo>::const_iterator iter = supported_formats.find(suffix);
	FormatHandler handler =
		(iter != supported_formats.end()) ? iter->second.handler : UNKNOWN;

	switch (handler)
	{
		case Q_IMAGE_READER:
			return load_qimage_file(
					file_path,
					read_errors);
		
		case GDAL:
			return load_gdal_file(
					file_path,
					read_errors);

		default:
			read_errors.d_failures_to_begin.push_back(
					make_read_error_occurrence(
						file_path,
						DataFormats::RasterImage,
						0,
						ReadErrors::UnrecognisedRasterFileType,
						ReadErrors::FileNotLoaded));
			return boost::none;
	}
}



void
GPlatesFileIO::RasterReader::populate_time_dependent_raster_map(
	   QMap<int, QString> &raster_map,
	   QString directory_path,
	   GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(directory_path, GPlatesFileIO::DataFormats::Unspecified));

		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumber(0));

		QDir directory(directory_path);
		QStringList filters;
		filters  << "*-*.jpg" << "*-*.jpeg";
		directory.setNameFilters(filters);
		QStringList file_list = directory.entryList(QDir::Files | QDir::NoDotAndDotDot);

		if (file_list.isEmpty()) {
			read_errors.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::NoRasterSetsFound,
				GPlatesFileIO::ReadErrors::NoRasterSetsLoaded));
			return;
		}

		QStringList::const_iterator it = file_list.constBegin();
		QStringList::const_iterator it_end = file_list.constEnd();
		
		QString accepted_file_root;
		QString potential_file_root;
		bool have_found_suitable_file = false;
		for (; it != it_end ; ++it)
		{
			int time = 0;
			if (parse_filename(*it,potential_file_root,time))
			{
				// We have a suitable file name.
				// Check if it's the first one we've come across.
				if (!have_found_suitable_file)
				{
					accepted_file_root = potential_file_root;
					raster_map.clear();
					raster_map.insert(time,directory_path+QDir::separator()+*it);
					have_found_suitable_file = true;
				}
#if 0				
// This demands that all file roots are the same, and complains if different roots
// exist in the folder.
				else if(accepted_file_root.compare(potential_file_root) == 0)
				// We already have a suitable file, make sure that our new file has the same root.
				{
					raster_map.insert(time,directory_path+QDir::separator()+*it);
				}
				else {
					// warn the user via the read-errors dialog
					read_errors.d_warnings.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							e_source,
							e_location,
							GPlatesFileIO::ReadErrors::MultipleRasterSetsFound,
							GPlatesFileIO::ReadErrors::OnlyFirstRasterSetLoaded));
				}
#else
// This accepts any file root, so that file roots in the same numerical sequence can be different.
// For example, the files
//	imageA-0.jpg
//	imageB-1.jpg
//	imageC-2.jpg
// ...
// would be accepted.
				else
				{
					raster_map.insert(time,directory_path+QDir::separator()+*it);			
				}
#endif
			}
		}
#if 0
	// Print out the raster map
		QMap<int, QString>::const_iterator map_it = raster_map.constBegin();
		QMap<int, QString>::const_iterator map_it_end = raster_map.constEnd();
		for ( ; map_it != map_it_end ; ++map_it)
		{
			qDebug() << map_it.key() << " " << map_it.value().toStdString().c_str();
		}
#endif
}


QString
GPlatesFileIO::RasterReader::get_nearest_raster_filename(
		const QMap<int, QString> &raster_map,
		double time)
{
	QString result;
	QMap<int, QString>::const_iterator it = raster_map.constBegin();
	QMap<int, QString>::const_iterator it_end = raster_map.constEnd();

	for (; (it != it_end) && (it.key() <= time) ; ++it)
	{}

	if ( it == raster_map.constBegin())
	{
		// The reconstruction time is later than the latest time in the file set. 
		result = it.value();
	}
	else if (it == it_end){
		// The reconstruction time is earlier than the earliest time in the file set. 
		result = (it-1).value();
	}
	else{
		// The reconstruction time lies between two consecutive times in the file set. 
		double dist1 = it.key() - time;
		double dist2 = time - (it-1).key();

		if (dist1 > dist2){
			result = (it-1).value();
		}
		else{
			result = it.value();
		}
	}

	return result;
}


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

