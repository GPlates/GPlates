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

#ifndef GPLATES_PRESENTATION_FILEIOINJECTIONS_H
#define GPLATES_PRESENTATION_FILEIOINJECTIONS_H

class QWidget;

namespace GPlatesPresentation
{
	/**
	 * Registers the GPlates-side implementations that the shared file-io code obtains by
	 * injection, because they need Qt Gui or Qt Widgets (which the pygplates module, compiled
	 * from the same file-io sources, does not link):
	 *
	 *  - the Qt-image-based reader for the RGBA raster formats (BMP, GIF, JPEG, PNG, SVG),
	 *    via @a GPlatesFileIO::RasterReader::set_rgba_reader_factory;
	 *  - the progress dialog shown while translating GeoSciML features,
	 *    via @a GPlatesFileIO::GeoscimlProfile::set_progress_reporter_factory;
	 *  - the shapefile attribute-mapping dialog, via @a GPlatesFileIO::OgrReader::set_property_mapper -
	 *    only when @a dialog_parent is non-null. Without it @a OgrReader falls back to a default
	 *    mapping, which is what the command-line interface has always done.
	 *
	 * Both entry points of the GPlates executable must call this before any file is read: the
	 * GUI (@a Application) and the command-line interface (@a gplates_main.cc), which never
	 * constructs an @a Application.
	 */
	void
	register_file_io_injections(
			QWidget *dialog_parent);
}

#endif  // GPLATES_PRESENTATION_FILEIOINJECTIONS_H
