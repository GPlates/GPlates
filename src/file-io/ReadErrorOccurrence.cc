/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "ReadErrorOccurrence.h"


const char *
GPlatesFileIO::DataFormats::data_format_to_str(
		DataFormat data_format)
{
	const char *str = NULL;

	switch (data_format) {
	case Gpml:
		str = "GPML";
		break;
	case PlatesRotation:
		str = "PLATES \"rotation\"";
		break;
	case PlatesLine:
		str = "PLATES \"line\"";
		break;
	case Shapefile:
		str = "ESRI Shapefile";
		break;
	case Gmap:
		str = "GMAP VGP";
		break;
	case RasterImage:
		str = "Raster image";
		break;
	case ScalarField3D:
		str = "3D scalar field";
		break;
	case Cpt:
		str = "GMT CPT";
		break;
	case Unspecified:
		str = "Unspecified";
		break;
	}

	return str;
}


GPlatesFileIO::ReadErrorOccurrence
GPlatesFileIO::make_read_error_occurrence(
		const QString &filename,
		DataFormats::DataFormat data_format,
		unsigned long line_num,
		ReadErrors::Description description,
		ReadErrors::Result result)
{
	return ReadErrorOccurrence(
			boost::shared_ptr<DataSource>(new LocalFileDataSource(filename, data_format)),
			boost::shared_ptr<LocationInDataSource>(new LineNumber(line_num)),
			description,
			result);
}


GPlatesFileIO::ReadErrorOccurrence
GPlatesFileIO::make_read_error_occurrence(
		boost::shared_ptr<DataSource> data_source,
		unsigned long line_num,
		ReadErrors::Description description,
		ReadErrors::Result result)
{
	return ReadErrorOccurrence(
			data_source,
			boost::shared_ptr<LocationInDataSource>(new LineNumber(line_num)),
			description,
			result);
}

