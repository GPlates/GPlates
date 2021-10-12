/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include <iostream>

#ifdef HAVE_CONFIG_H
// We're building on a UNIX-y system, and can thus expect "global/config.h".

// On some systems, it's <ogrsf_frmts.h>, on others, <gdal/ogrsf_frmts.h>.
// The "CMake" script should have determined which one to use.
#include "global/config.h"
#ifdef HAVE_GDAL_OGRSF_FRMTS_H
#include <gdal/gdal_priv.h>
#else
#include <gdal_priv.h>
#endif

#else  // We're not building on a UNIX-y system.  We'll have to assume it's <ogrsf_frmts.h>.
#include <gdal_priv.h>
#endif  // HAVE_CONFIG_H

// Use this header instead of <cmath> if you want to use 'std::isnan'.
#include <cmath_ext.h>

#include <QtOpenGL/qgl.h>
#include <QColor>
#include <QDir>
#include <QRgb>

#include "utils/MathUtils.h"
#include "ErrorOpeningFileForReadingException.h"
#include "GdalReader.h"

namespace{

	struct GdalException{};

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
	convert_data_to_qcolor(
		float data,
		QColor &colour,
		const float min,
		const float max)
	{
		if (data < min) data = min;
		if (data > max) data = max;
		float range = max - min;

		// The B->C->G->Y->R colour range is made up of 4 intervals.
		
		float r0 = min + 0.25*range;
		float r1 = min + 0.5*range;
		float r2 = min + 0.75*range;

		float dr = 0.25*range;

		// First interval is Blue to Cyan.
		if (data < r0)
		{
			colour.setRed(0);
			colour.setGreen(static_cast<int>((data-min)/dr*255.));
			colour.setBlue(255);
		}
		// Second interval is Cyan to Green.
		else if (data < r1)
		{
			colour.setRed(0);
			colour.setGreen(255);		
			colour.setBlue(static_cast<int>(255.-(data-r0)/dr*255.));
		}
		// Third interval is Green to Yellow.
		else if (data < r2)
		{
			colour.setRed(static_cast<int>((data-r1)/dr*255));
			colour.setGreen(255);		
			colour.setBlue(0);
		}
		// Fourth interval is Yellow to Red.
		else 
		{
			colour.setRed(255);
			colour.setGreen(static_cast<int>(255.-(data-r2)/dr*255.));
			colour.setBlue(0);
		}
	}

	void
	convert_line_to_RGB(
		std::vector<float> &line,
		std::vector<GLubyte> &rgb,
		double colour_min,
		double colour_max,
		double no_data_value,
		int no_data_value_success)
	{
			std::vector<float>::iterator it = line.begin();
			std::vector<float>::iterator it_end = line.end();

			float data;
			QColor colour;
			// loop over the pixels in the scan line and convert
			// them to colour indices
			for ( ; it != it_end ; ++it)
			{
				data = *it;
				if (std::isnan(data)){
					std::cerr << "NaN detected" << std::endl;
					colour = Qt::black;
				}
				else if (no_data_value_success && (GPlatesUtils::are_almost_exactly_equal(static_cast<double>(data),no_data_value))){
			//		std::cerr << "no_data_value detected" << std::endl;
					colour = Qt::black;
				}
				else{
					convert_data_to_qcolor(data,colour,colour_min,colour_max);
				}
				rgb.push_back(colour.red());
				rgb.push_back(colour.green());
				rgb.push_back(colour.blue());
				rgb.push_back(static_cast<GLubyte>(255));
			}
	}


	void
	convert_raster_to_RGB(
		GDALRasterBand *band,
		std::vector<GLubyte> &rgb,
		double colour_min,
		double colour_max,
		bool flip)
	{
		if (band == NULL) return;

		int width = band->GetXSize();
		int height = band->GetYSize();
	
		// The scanline will be stored here.
		std::vector<float> line(width);
		int line_index;
	
		int no_data_value_success;
		double no_data_value = band->GetNoDataValue(&no_data_value_success);

		for (int i = 0; i < height; i ++)
		{
	
			line_index = i;
			if (flip) {
				line_index = height-1-i;
			}
			CPLErr error = band->RasterIO(GF_Read,
				0,
				line_index,
				width,
				1,
				&line[0],
				width,
				1,
				GDT_Float32,
				0,
				0);
			
			if (error != CE_None){
				throw(GPlatesFileIO::ReadErrors::ErrorReadingGDALBand);
			}
			convert_line_to_RGB(line,rgb,colour_min,colour_max,no_data_value,no_data_value_success);
		}		

	}



	void
	display_gdal_projection_info(
		GDALDataset *dataset)
	{
	
		if (dataset == NULL)
		{
			return;
		}
#if 0
		std::cerr << "Driver: " <<
			dataset->GetDriver()->GetDescription() << " " << 
			dataset->GetDriver()->GetMetadataItem(GDAL_DMD_LONGNAME) 
			<< std::endl;

		if ( dataset->GetProjectionRef() != NULL)
		{
			std::cerr << dataset->GetProjectionRef() ;
		}

		if ( dataset->GetGeoTransform(adfGeoTransform) == CE_None)
		{
			double adfGeoTransform[6]; 
			std::cerr << "Origin: " <<
				adfGeoTransform[0] << " " << adfGeoTransform[3] << std::endl;
			std::cerr << "Pixel size: " <<
				adfGeoTransform[1] << " " << adfGeoTransform[5] << std::endl;
		}
#endif
	}

	void
	display_gdal_band_info(
		GDALRasterBand *band)
	{

		int xsize = band->GetXSize();
		int ysize = band->GetYSize();

		std::cerr << "X size: " << xsize << ", Y size: " << ysize << std::endl;

		int block_x, block_y;
		band->GetBlockSize(&block_x,&block_y);
		
		std::cerr << "Block size- x: " << block_x << " y: " << block_y << std::endl;
		std::cerr << "Type: " << 
			GDALGetDataTypeName(band->GetRasterDataType()) << std::endl;
		std::cerr << "Color Interpretation: " << 
			GDALGetColorInterpretationName(band->GetColorInterpretation()) << std::endl;

		std::cerr << "Has arbitrary overviews: " << band->HasArbitraryOverviews() << std::endl;

		int num_overviews = band->GetOverviewCount();
		std::cerr << "Overview count: " << num_overviews << std::endl;

		for (int count = 0; count < num_overviews ; count ++)
		{
			GDALRasterBand *temp_band = band->GetOverview(count);
			std::cerr << std::endl;
			std::cerr << "Band information about overview " << count << std::endl;
			display_gdal_band_info(temp_band);
		}

		int got_min, got_max; 
		double adfMinMax[2];

		adfMinMax[0] = band->GetMinimum(&got_min);
		adfMinMax[1] = band->GetMaximum(&got_max);

		if (! (got_min && got_max)){
			GDALComputeRasterMinMax(static_cast<GDALRasterBandH>(band), TRUE, adfMinMax);
		}

		std::cerr << "Min: " << adfMinMax[0] << ", Max: " << adfMinMax[1] << std::endl;

		if (band->GetColorTable() != NULL)
		{
			std::cerr << "Band has color table with " << 
				band->GetColorTable()->GetColorEntryCount() << " entries." << std::endl;
		}
	}
} // anonymous namespace

GPlatesFileIO::GdalReader::GdalReader()
{
	GDALAllRegister();

	d_dataset_ptr = NULL;
}

GPlatesFileIO::GdalReader::~GdalReader()
{
	try {
		if (d_dataset_ptr) {
			GDALClose(d_dataset_ptr);
		}
	}
	catch (...) {
	}
}

void
GPlatesFileIO::GdalReader::read_file(
	 const QString &filename,
	 GPlatesPropertyValues::InMemoryRaster &raster,
	 GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Unspecified));
	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
		new GPlatesFileIO::LineNumberInFile(0));

	d_dataset_ptr = static_cast<GDALDataset *>(GDALOpen(filename.toStdString().c_str(), GA_ReadOnly));
	
	if (d_dataset_ptr == NULL)
	{
	//	std::cerr << "Error opening dataset via GDAL." << std::endl;
		throw GPlatesFileIO::ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	display_gdal_projection_info(d_dataset_ptr);

	int n = d_dataset_ptr->GetRasterCount();

	//std::cerr << n << " Raster band(s) in file." << std::endl;

	if (n == 0) {
		read_errors.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorReadingGDALBand,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
		return;
	}

	// GDAL raster bands are numbered from 1. 
	GDALRasterBand *gdal_band_ptr = d_dataset_ptr->GetRasterBand(1);
	if (gdal_band_ptr == NULL) {
		read_errors.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorReadingGDALBand,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
		return;
	}
#if 0
	display_gdal_band_info(gdal_band_ptr);
#endif

	QSize size(gdal_band_ptr->GetXSize(),gdal_band_ptr->GetYSize());

	std::vector<GLubyte> image_RGB;

	double max,min,mean,dev;

	if (gdal_band_ptr->GetStatistics(false,true,&min,&max,&mean,&dev) != CE_None)
	{
		read_errors.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			e_source,
			e_location,
			GPlatesFileIO::ReadErrors::ErrorReadingGDALBand,
			GPlatesFileIO::ReadErrors::FileNotLoaded));
		return;		
	}

	// Set up colour_min and colour_max so that the colour scale covers the 
	// range (mean - 2*standard_deviation) to (mean + 2*standard_deviation)
	double colour_min = mean - 2*dev;
	double colour_max = mean + 2*dev;


	// GMT style GRDs are stored, and imported, upside-down.
	// See for example http://trac.osgeo.org/gdal/ticket/1926
	bool flip = false;
	QString driver_type = d_dataset_ptr->GetDriver()->GetDescription();
	if (driver_type.compare("GMT") == 0){
		flip = true;
	}

	convert_raster_to_RGB(gdal_band_ptr,image_RGB,colour_min,colour_max,flip);

	raster.set_min(colour_min);
	raster.set_max(colour_max);
	raster.set_corresponds_to_data(true);
	raster.generate_raster(image_RGB,size,
		GPlatesPropertyValues::InMemoryRaster::RgbaFormat);
	raster.set_enabled(true);

}
