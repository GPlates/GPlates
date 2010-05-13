/* $Id$ */

/**
 * \file 
 * Contains the definition of the class CategoricalCptReader.
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

#ifndef GPLATES_FILEIO_CATEGORICALCPTREADER_H
#define GPLATES_FILEIO_CATEGORICALCPTREADER_H

#include <boost/shared_ptr.hpp>
#include <QString>
#include <QTextStream>

#include "ReadErrorOccurrence.h"


namespace GPlatesGui
{
	template<typename T> class CategoricalCptColourPalette;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	/**
	 * This reads in GMT colour palette tables (CPT) files.
	 *
	 * This version reads the "categorical" kind, which maps a set of discrete
	 * values to colours. Categorical CPT files have lines of the form:
	 *
	 *		key fill label
	 *
	 * Although the documentation on categorical CPT files is vague and there are
	 * no samples, it appears that the "key" component is an integer, which
	 * creates difficulties if we want to colour by non-numerical properties.
	 *
	 * GPlates, therefore, interprets these lines differently depending on the
	 * target value type. If we wish to map integers to colours, the key is taken
	 * as the value that we map to the colour. If we wish to map any other type to
	 * a colour, the label, parsed accordingly, is taken as the value that we map
	 * to the colour, and the key is merely used to indicate sorting order.
	 *
	 * The target value type is specified as the T template parameter.
	 *
	 * A description of a "categorical" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node68.html 
	 *
	 * This reader does not understand pattern fills.
	 *
	 * This reader also does not respect the .gmtdefaults4 settings file.
	 */
	template<typename T>
	class CategoricalCptReader
	{
	public:

		/**
		 * Parsers text from the provided @a text_stream as a categorical CPT file.
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
		GPlatesGui::CategoricalCptColourPalette<T> *
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
		 * a categorical CPT file.
		 *
		 * @see read_file() that takes a QTextStream as the first parameter.
		 */
		GPlatesGui::CategoricalCptColourPalette<T> *
		read_file(
				const QString &filename,
				ReadErrorAccumulation &errors) const;
	};
}

#endif  // GPLATES_FILEIO_CATEGORICALCPTREADER_H
