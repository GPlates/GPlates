/* $Id$ */

/**
 * \file 
 * Contains the definition of the class RegularCptReader.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.h")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "RegularCptReader.h")
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

#ifndef GPLATES_FILEIO_REGULARCPTREADER_H
#define GPLATES_FILEIO_REGULARCPTREADER_H

#include <QString>


namespace GPlatesGui
{
	class GenericContinuousColourPalette;
}

namespace GPlatesFileIO
{
	struct ErrorReadingCptFileException
	{
	};

	/**
	 * This reads in GMT colour palette tables (CPT) files.
	 *
	 * This version reads the "regular" kind, which consists of a series of
	 * continuous ranges with colours linearly interpolated between the ends of
	 * these ranges. (The other kind is "categorical" CPT files, used where it
	 * makes no sense to interpolate between values; the values are discrete.)
	 */
	class RegularCptReader
	{
	public:

		/**
		 * Reads the CPT file given by @a filename, storing the results into @palette.
		 */
		void
		read_file(
				QString filename,
				GPlatesGui::GenericContinuousColourPalette &palette);
	};
}

#endif  // GPLATES_FILEIO_REGULARCPTREADER_H
