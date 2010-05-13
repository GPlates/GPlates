/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
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

// On some versions of g++ with some versions of Qt, it's not liking at() and
// operator[] in QStringList.
#if defined(__GNUC__)
#	pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

#include <boost/noncopyable.hpp>
#include <QFile>
#include <QStringList>

#include "CategoricalCptReader.h"
#include "CptReaderUtils.h"
#include "ReadErrorAccumulation.h"

#include "gui/CptColourPalette.h"

#include "maths/Real.h"


using boost::tuples::get;
using GPlatesMaths::Real;

namespace
{
	using namespace GPlatesFileIO;

#if 0
	/**
	 * Attempts to parse a line in a regular CPT file.
	 *
	 * @a parser_state.any_successful_line is set to true if the @a line was
	 * successfully parsed as a non-comment line.
	 */
	void
	try_process_line(
			const QString &line,
			ParserState &parser_state)
	{
		if (try_process_comment(line, parser_state))
		{
			return;
		}

		// Split the string by whitespace.
		QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
	}
#endif
}


template<typename T>
GPlatesGui::CategoricalCptColourPalette<T> *
GPlatesFileIO::CategoricalCptReader<T>::read_file(
		QTextStream &text_stream,
		ReadErrorAccumulation &errors,
		boost::shared_ptr<DataSource> data_source) const
{
	GPlatesGui::CategoricalCptColourPalette<T> *palette = new GPlatesGui::CategoricalCptColourPalette<T>();
#if 0
	ParserState parser_state(*palette, errors, data_source);

	// Go through each line one by one.
	while (!text_stream.atEnd())
	{
		++parser_state.current_line_number;
		QString line = text_stream.readLine().trimmed();

		if (!line.isEmpty())
		{
			try_process_line(line, parser_state);
		}
	}
	
	if (parser_state.any_successful_lines)
	{
		// Remember whether the file was read using the RGB or HSV colour model.
		palette->set_rgb_colour_model(parser_state.rgb);

		return palette;
	}
	else
	{
		// We add an error and return NULL if we did not parse any lines at all.
		errors.d_terminating_errors.push_back(
				make_read_error_occurrence(
					data_source,
					0,
					ReadErrors::NoLinesSuccessfullyParsed,
					ReadErrors::FileNotLoaded));

		return NULL;
	}
#endif
	return palette;
}


template<typename T>
GPlatesGui::CategoricalCptColourPalette<T> *
GPlatesFileIO::CategoricalCptReader<T>::read_file(
		const QString &filename,
		ReadErrorAccumulation &errors) const
{
	QFile qfile(filename);
	boost::shared_ptr<DataSource> data_source(
			new LocalFileDataSource(
				filename,
				DataFormats::Cpt));

	if (qfile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// File open succeeded, proceed to read the file.
		QTextStream text_stream(&qfile);
		return read_file(text_stream, errors, data_source);
	}
	else
	{
		// File could not be opened for reading, add error and return NULL.
		errors.d_failures_to_begin.push_back(
				make_read_error_occurrence(
					data_source,
					0,
					ReadErrors::ErrorOpeningFileForReading,
					ReadErrors::FileNotLoaded));
		return NULL;
	}
}

