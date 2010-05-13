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

#include <boost/shared_ptr.hpp>
#include <QString>
#include <QTextStream>

#include "ReadErrorOccurrence.h"


namespace GPlatesGui
{
	class RegularCptColourPalette;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	/**
	 * This reads in GMT colour palette tables (CPT) files.
	 *
	 * This version reads the "regular" kind, which consists of a series of
	 * continuous ranges with colours linearly interpolated between the ends of
	 * these ranges. (The other kind is "categorical" CPT files, used where it
	 * makes no sense to interpolate between values; the values are discrete.)
	 *
	 * A description of a "regular" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node69.html 
	 *
	 * This reader does not understand pattern fills.
	 *
	 * This reader also does not respect the .gmtdefaults4 settings file.
	 */
	class RegularCptReader
	{
	public:

		/**
		 * Parsers text from the provided @a text_stream as a regular CPT file.
		 *
		 * Ownership of the memory pointed to by the returned pointer passes to the
		 * called of this function and it is the responsibility of the caller to make
		 * sure that the memory is deallocated correctly.
		 *
		 * Returns NULL if the entire file provided contained no lines recognised as
		 * belonging to a regular CPT file.
		 *
		 * Any errors will be added to the @a errors accumulator.
		 */
		GPlatesGui::RegularCptColourPalette *
		read_file(
				QTextStream &text_stream,
				ReadErrorAccumulation &errors,
				boost::shared_ptr<DataSource> data_source =
					boost::shared_ptr<DataSource>(
						new GenericDataSource(
							DataFormats::Cpt,
							"QTextStream"))) const;

		/**
		 * A convenience function for reading the file with the given @a filename as
		 * a regular CPT file.
		 *
		 * @see read_file() that takes a QTextStream as the first parameter.
		 */
		GPlatesGui::RegularCptColourPalette *
		read_file(
				const QString &filename,
				ReadErrorAccumulation &errors) const;
	};
}

#endif  // GPLATES_FILEIO_REGULARCPTREADER_H
