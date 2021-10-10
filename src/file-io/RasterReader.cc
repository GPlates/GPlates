/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#include <QDir>
#include <QImageReader>

#include "gui/Texture.h"
#include "utils/MathUtils.h"

#include "ErrorOpeningFileForReadingException.h"
#include "GdalReader.h"
#include "ReadErrorAccumulation.h"
#include "RasterReader.h"


namespace{
	
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
		
		QString filename_stripped = filename.section('.',-2,-2);

		// Split into <root> and <time> parts.
		QStringList parts = filename_stripped.split(QString("-"));	
		if (parts.size() != 2)
		{
			std::cerr << "Filename is not of required form." << std::endl;
			return false;
		}
		QString potential_root = parts.at(0);
		QString num_string = parts.at(1);
		bool ok;
		time = num_string.toInt(&ok);
		if (!ok){
			std::cerr << "Second field cannot be converted to an integer." << std::endl;
			return false;
		}
		
		root = potential_root;
		return true;
	}

	// a blue->cyan->green->yellow->red colour map.
	const static QRgb colour_map[9] = {
		qRgb(0,0,255),			// blue = low
		qRgb(0,127,255),
		qRgb(0,255,255),		// Cyan
		qRgb(0,255,127),
		qRgb(0,255,0),			// green = central   
		qRgb(127,255,0),
		qRgb(255,255,0),		// yellow
		qRgb(255,127,0),
		qRgb(255,0,0)    // red = high
	};

	const float NUM_BANDS = 7;
	const float BAND_MIN = -1800.;
	const float BAND_MAX = 1800.;
	const float BAND_WIDTH =(BAND_MAX - BAND_MIN)/NUM_BANDS;

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

		int size;

		FILE *tga_file;

		if (!(tga_file = fopen (filename.toStdString().c_str(), "rb")))
			return;

		// skip over some header data
		fread (&temp_char, sizeof (unsigned char), 1, tga_file);
		fread (&temp_char, sizeof (unsigned char), 1, tga_file);

		// get the type, and make sure it's RGB
		fread (&type, sizeof (unsigned char), 1, tga_file);

		if (type != 2) return;

		// skip over some more header data
		fread (&temp_short,sizeof(unsigned short),1,tga_file);
		fread (&temp_short,sizeof(unsigned short),1,tga_file);
		fread (&temp_char, sizeof (unsigned char), 1, tga_file);
		fread (&temp_short,sizeof(unsigned short),1,tga_file);
		fread (&temp_short,sizeof(unsigned short),1,tga_file);

		// read the width, height, and bits-per-pixel.
		fread(&width,sizeof(unsigned short),1,tga_file);
		fread(&height,sizeof(unsigned short),1,tga_file);
		fread(&bpp,sizeof(unsigned char),1,tga_file);

		fread(&temp_char,sizeof(unsigned char),1,tga_file);

		if (bpp != 24){
			return;
		}

		size = width * height; 

		std::vector<GLubyte> image_data_vector(size*3);

		int num_read = fread(&image_data_vector[0],sizeof (unsigned char),size*3,tga_file);

		fclose (tga_file);

		if (num_read != size*3){
			return;
		}

		// change BGR to RGB
		GLubyte temp;
		for (int i = 0; i < size * 3; i += 3)
		{
			temp = image_data_vector[i];
			image_data_vector[i] = image_data_vector[i + 2];
			image_data_vector[i + 2] = temp;
		}

		GPlatesPropertyValues::InMemoryRaster::ColourFormat format =
				GPlatesPropertyValues::InMemoryRaster::RgbFormat;
		raster.set_corresponds_to_data(false);
		raster.generate_raster(image_data_vector,
								  width,
								  height,
								  format);

		raster.set_enabled(true);
	}

	void
	load_qimage_file(
		const QString &filename,
		GPlatesPropertyValues::InMemoryRaster &raster,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumberInFile(0));

		QImageReader image_reader(filename);
		QImage raw_image = image_reader.read();
		if (raw_image.isNull()){
	//		std::cerr << image_reader.errorString().toStdString().c_str() << std::endl;
			read_errors.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorReadingQImageFile,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
			return;
		}


		int width = raw_image.width();
		int height = raw_image.height();
		
	//	std::cerr << "Jpg dimensions: width: " << width << ", height: " << height << std::endl;
		QImage image = QGLWidget::convertToGLFormat(raw_image);

		GPlatesPropertyValues::InMemoryRaster::ColourFormat format =
				GPlatesPropertyValues::InMemoryRaster::RgbaFormat;

		raster.set_corresponds_to_data(false);
		raster.generate_raster(
			image.bits(),
			width,
			height,
			format);
		raster.set_enabled(true);
	}

	void
	load_gdal_file(
		const QString &filename,
		GPlatesPropertyValues::InMemoryRaster &raster,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{

		GPlatesFileIO::GdalReader reader;
		
		reader.read_file(filename,raster,read_errors);

	}
} // anonymous namespace

void
GPlatesFileIO::RasterReader::read_file(
	   FileInfo &file_info,
	   GPlatesPropertyValues::InMemoryRaster &raster,
	   ReadErrorAccumulation &read_errors)
{
	QFileInfo q_file_info = file_info.get_qfileinfo();

	QString suffix = q_file_info.suffix();

#if 0
	if (suffix.compare(QString("tga"),Qt::CaseInsensitive) == 0)
	{
	//	std::cerr << "TGA file." << std::endl;
		load_tga_file(file_info.get_qfileinfo().absoluteFilePath(),
						raster,
						read_errors);
	}
#endif
	if ((suffix.compare(QString("jpg"),Qt::CaseInsensitive) == 0) ||
		(suffix.compare(QString("jpeg"),Qt::CaseInsensitive) == 0))
	{
	//	std::cerr << "JPG/JPEG file." << std::endl;

		load_qimage_file(file_info.get_qfileinfo().absoluteFilePath(),
						raster,
						read_errors);
	}
	else if (suffix.compare(QString("grd"),Qt::CaseInsensitive) == 0)
	{
	//	std::cerr << "GRD file." << std::endl;
		load_gdal_file(file_info.get_qfileinfo().absoluteFilePath(),
						raster,
						read_errors);

	}
	else
	{
		// FIXME: add to read errors.
		std::cerr << "Unrecognised file type for file: " << q_file_info.absoluteFilePath().toStdString() << std::endl;
	}
}

void
GPlatesFileIO::RasterReader::populate_time_dependent_raster_map(
	   QMap<int,QString> &raster_map,
	   QString directory_path,
	   GPlatesFileIO::ReadErrorAccumulation &read_errors)
{

		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
			new GPlatesFileIO::LocalFileDataSource(directory_path, GPlatesFileIO::DataFormats::Unspecified));

		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
			new GPlatesFileIO::LineNumberInFile(0));

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
			// std::cerr << "No files in directory..." << std::endl;
			return;
		}

		QStringList::const_iterator it = file_list.constBegin();
		QStringList::const_iterator it_end = file_list.constEnd();
		
		QString accepted_file_root;
		QString potential_file_root;
		bool have_found_suitable_file = false;
		for (; it != it_end ; ++it)
		{
			int time;
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
			}
		}
#if 0
	// Print out the raster map
		QMap<int,QString>::const_iterator map_it = raster_map.constBegin();
		QMap<int,QString>::const_iterator map_it_end = raster_map.constEnd();
		for ( ; map_it != map_it_end ; ++map_it)
		{
			std::cerr << map_it.key() << " " << map_it.value().toStdString().c_str() << std::endl;
		}
#endif
}


QString
GPlatesFileIO::RasterReader::get_nearest_raster_filename(
		const QMap<int,QString> &raster_map,
		double time)
{
	QString result;
	QMap<int,QString>::const_iterator it = raster_map.constBegin();
	QMap<int,QString>::const_iterator it_end = raster_map.constEnd();

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

	//std::cerr << result.toStdString().c_str() << std::endl;
	return result;
}

