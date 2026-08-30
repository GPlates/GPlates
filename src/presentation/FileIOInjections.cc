/* $Id$ */

/**
 * @file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#include <boost/shared_ptr.hpp>
#include <QtGlobal>

#include "FileIOInjections.h"

#include "file-io/OgrReader.h"
#include "file-io/RasterReader.h"
#include "file-io/RgbaRasterReader.h"
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include "file-io/GeoscimlProfile.h"
#endif

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include "qt-widgets/GeoscimlProgressDialog.h"
#endif
#include "qt-widgets/ShapefilePropertyMapper.h"


namespace
{
	GPlatesFileIO::RasterReaderImpl *
	create_rgba_raster_reader(
			const QString &filename,
			GPlatesFileIO::RasterReader *raster_reader,
			GPlatesFileIO::ReadErrorAccumulation *read_errors)
	{
		return new GPlatesFileIO::RgbaRasterReader(filename, raster_reader, read_errors);
	}

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
	boost::shared_ptr<GPlatesFileIO::GeoscimlProfile::ProgressReporter>
	create_geosciml_progress_dialog()
	{
		return boost::shared_ptr<GPlatesFileIO::GeoscimlProfile::ProgressReporter>(
				new GPlatesQtWidgets::GeoscimlProgressDialog());
	}
#endif
}


void
GPlatesPresentation::register_file_io_injections(
		QWidget *dialog_parent)
{
	GPlatesFileIO::RasterReader::set_rgba_reader_factory(&create_rgba_raster_reader);

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
	GPlatesFileIO::GeoscimlProfile::set_progress_reporter_factory(&create_geosciml_progress_dialog);
#endif

	if (dialog_parent)
	{
		boost::shared_ptr<GPlatesQtWidgets::ShapefilePropertyMapper> shapefile_property_mapper(
				new GPlatesQtWidgets::ShapefilePropertyMapper(dialog_parent));
		GPlatesFileIO::OgrReader::set_property_mapper(shapefile_property_mapper);
	}
}
