/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.cc")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "RegularCptReader.cc")
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

#include "CptReaderUtils.h"
#include "ReadErrorAccumulation.h"
#include "RegularCptReader.h"

#include "gui/CptColourPalette.h"

#include "maths/Real.h"


using boost::tuples::get;
using GPlatesMaths::Real;

namespace
{
	using namespace GPlatesFileIO;

	/**
	 * Stores the state of the regular CPT parser as it proceeds through the file.
	 */
	struct ParserState :
			public boost::noncopyable
	{
		ParserState(
				GPlatesGui::RegularCptColourPalette &palette_,
				ReadErrorAccumulation &errors_,
				boost::shared_ptr<DataSource> data_source_) :
			palette(palette_),
			errors(errors_),
			data_source(data_source_),
			rgb(true),
			any_successful_lines(false),
			current_line_number(0),
			upper_value_of_previous_slice(GPlatesMaths::negative_infinity())
		{
		}

		/**
		 * The data structure that holds all lines successfully read in.
		 */
		GPlatesGui::RegularCptColourPalette &palette;

		/**
		 * For the reporting of read errors.
		 */
		ReadErrorAccumulation errors;

		/**
		 * Where our lines are coming from; used for error reporting.
		 */
		boost::shared_ptr<DataSource> data_source;

		/**
		 * The default is true. If false, the colour model is HSV.
		 */
		bool rgb;

		/**
		 * True if any non-comment lines have been successfully parsed.
		 */
		bool any_successful_lines;

		/**
		 * The line number that we're currently parsing.
		 */
		unsigned long current_line_number;

		/**
		 * Stores the upper z-value of the previous slice.
		 */
		double upper_value_of_previous_slice;
	};


	/**
	 * Attempts to process a line as a comment.
	 *
	 * This function also checks if the comment is a special comment that switches
	 * the colour model to RGB or HSV.
	 *
	 * Returns true if successful. Note that this function does not set the
	 * any_successful_lines variable in @a parser_state because the notion of
	 * successful lines only includes successful non-comment lines.
	 */
	bool
	try_process_comment(
			const QString &line,
			ParserState &parser_state)
	{
		// RGB or +RGB or HSV or +HSV.
		static const QRegExp rgb_regex("\\+?RGB");
		static const QRegExp hsv_regex("\\+?HSV");

		if (line.startsWith("#"))
		{
			// Remove the # and see if the resulting comment is a colour model statement.
			QString comment = line.right(line.length() - 1);
			QStringList tokens = comment.split(QRegExp("[=\\s+]"), QString::SkipEmptyParts);
			if (tokens.count() == 2 && tokens.at(0) == "COLOR_MODEL")
			{
				if (rgb_regex.exactMatch(tokens.at(1)))
				{
					parser_state.rgb = true;
				}
				else if (hsv_regex.exactMatch(tokens.at(1)))
				{
					parser_state.rgb = false;
				}
				else
				{
					// It's not a colour model statement, but it's still a valid comment.
					return true;
				}

				// Warn user if colour model statement occurs after some lines have already
				// been processed; we will begin to process colours in lines following this
				// one using the new colour model, but this probably not what the user
				// intended.
				if (parser_state.any_successful_lines)
				{
					parser_state.errors.d_warnings.push_back(
							make_read_error_occurrence(
								parser_state.data_source,
								parser_state.current_line_number,
								ReadErrors::ColourModelChangedMidway,
								ReadErrors::NoAction));
				}
			}
			
			// A valid comment.
			return true;
		}
		else
		{
			// Not a valid comment.
			return false;
		}
	}


	/**
	 * Parses the optional label at the end of a regular CPT file line. The label
	 * starts with a semi-colon.
	 */
	boost::optional<QString>
	parse_label(
			const QString &token)
	{
		if (token.startsWith(';'))
		{
			return token.right(token.length() - 1);
		}
		else
		{
			throw CptReaderUtils::BadTokenException();
		}
	}


	/**
	 * Attempts to process a regular CPT file line as a colour slice.
	 *
	 * The format of the line is:
	 *
	 *		lower_value R G B upper_value R G B [a] [;label]
	 *
	 * if the ColourSpecification parameter is CptReaderUtils::RGBColourSpecification.
	 * For other choices of ColourSpecification, R, G and B are replaced as appropriate.
	 *
	 * If the line was successfully parsed, the function returns true and inserts a
	 * new entry into the current colour palette.
	 */
	template<class ColourSpecification>
	bool
	try_process_colour_slice(
			const QStringList &tokens,
			ParserState &parser_state)
	{
		typedef ColourSpecification colour_specification_type;
		typedef typename colour_specification_type::components_type components_type;
		static const int NUM_COMPONENTS = boost::tuples::length<components_type>::value;

		// Check that the tokens list has an appropriate length: it must have
		// the compulsory elements, with 2 optional tokens at the end.
		static const int MIN_TOKENS_COUNT = (1 + NUM_COMPONENTS) * 2;
		static const int MAX_TOKENS_COUNT = MIN_TOKENS_COUNT + 2;
		if (tokens.count() < MIN_TOKENS_COUNT || tokens.count() > MAX_TOKENS_COUNT)
		{
			return false;
		}

		try
		{
			// Lower value of z-slice.
			double lower_value = CptReaderUtils::parse_token<double>(tokens.at(0));

			// First lot of colour components.
			components_type lower_components = CptReaderUtils::parse_components<components_type>(tokens, 1);
			boost::optional<GPlatesGui::Colour> lower_colour = colour_specification_type::convert(lower_components);

			// Upper value of z-slice.
			double upper_value = CptReaderUtils::parse_token<double>(tokens.at(1 + NUM_COMPONENTS));

			// Second lot of colour components.
			components_type upper_components = CptReaderUtils::parse_components<components_type>(
					tokens, 1 + NUM_COMPONENTS + 1);
			boost::optional<GPlatesGui::Colour> upper_colour = colour_specification_type::convert(upper_components);

			// Parse the last 2 tokens, if any.
			GPlatesGui::ColourScaleAnnotation::Type annotation = (tokens.count() == MIN_TOKENS_COUNT) ?
				GPlatesGui::ColourScaleAnnotation::NONE :
				CptReaderUtils::parse_token<GPlatesGui::ColourScaleAnnotation::Type>(tokens.at(MIN_TOKENS_COUNT));
			boost::optional<QString> label = (tokens.count() == MAX_TOKENS_COUNT) ?
				parse_label(tokens.at(MAX_TOKENS_COUNT - 1)) :
				boost::none;
			
			// Issue a warning if this slice does not start after the end of the previous slice.
			if (Real(lower_value) < Real(parser_state.upper_value_of_previous_slice))
			{
				parser_state.errors.d_warnings.push_back(
						make_read_error_occurrence(
							parser_state.data_source,
							parser_state.current_line_number,
							ReadErrors::CptSliceNotMonotonicallyIncreasing,
							ReadErrors::NoAction));
				parser_state.upper_value_of_previous_slice = upper_value;
			}

			// Store in palette.
			parser_state.palette.add_entry(
					GPlatesGui::ColourSlice(
						lower_value,
						lower_colour,
						upper_value,
						upper_colour,
						annotation,
						label));

			return true;
		}
		catch (const CptReaderUtils::BadTokenException &)
		{
			return false;
		}
		catch (const CptReaderUtils::BadComponentsException &)
		{
			return false;
		}
		catch (const CptReaderUtils::PatternFillEncounteredException &)
		{
			parser_state.errors.d_warnings.push_back(
					make_read_error_occurrence(
						parser_state.data_source,
						parser_state.current_line_number,
						ReadErrors::PatternFillInLine,
						ReadErrors::CptLineIgnored));

			return false;
		}
	}


	/**
	 * Delegates to the correct function depending on the current colour model.
	 */
	bool
	try_process_rgb_or_hsv_colour_slice(
			const QStringList &tokens,
			ParserState &parser_state)
	{
		if (parser_state.rgb)
		{
			return try_process_colour_slice<CptReaderUtils::RGBColourSpecification>(tokens, parser_state);
		}
		else
		{
			return try_process_colour_slice<CptReaderUtils::HSVColourSpecification>(tokens, parser_state);
		}
	}


	/**
	 * Attempts to process a regular CPT file line as a "FBN" line.
	 *
	 * The format of the line is one of:
	 *
	 *		F	R	G	B
	 *		B	R	G	B
	 *		N	R	G	B
	 * 
	 * if the ColourSpecification parameter is CptReaderUtils::RGBColourSpecification.
	 * The only other valid ColourSpecification for a FBN line is
	 * CptReaderUtils::HSVColourSpecification.
	 *
	 * If the line was successfully parsed, the function returns true and changes
	 * the foreground, background or NaN colours in the colour palette as appropriate.
	 */
	template<class ColourSpecification>
	bool
	try_process_fbn(
			const QStringList &tokens,
			ParserState &parser_state)
	{
		typedef ColourSpecification colour_specification_type;
		typedef typename colour_specification_type::components_type components_type;
		static const int NUM_COMPONENTS = boost::tuples::length<components_type>::value;

		// Check that the tokens list is of the right length; it must be one longer
		// than the number of components in the colour.
		static const int EXPECTED_TOKENS_COUNT = 1 + NUM_COMPONENTS;
		if (tokens.count() != EXPECTED_TOKENS_COUNT)
		{
			return false;
		}

		try
		{
			// Convert the colour, which starts from token 1.
			components_type colour_components = CptReaderUtils::parse_components<components_type>(tokens, 1);
			boost::optional<GPlatesGui::Colour> colour = colour_specification_type::convert(colour_components);
			if (!colour)
			{
				return false;
			}

			// The first character, which is B, F or N.
			const QString &letter = tokens.at(0);
			if (letter == "B")
			{
				parser_state.palette.set_background_colour(*colour);
				return true;
			}
			else if (letter == "F")
			{
				parser_state.palette.set_foreground_colour(*colour);
				return true;
			}
			else if (letter == "N")
			{
				parser_state.palette.set_nan_colour(*colour);
				return true;
			}
			else
			{
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
	}


	/**
	 * Delegates to the correct function depending on the current colour model.
	 */
	bool
	try_process_rgb_or_hsv_fbn(
			const QStringList &tokens,
			ParserState &parser_state)
	{
		if (parser_state.rgb)
		{
			return try_process_fbn<CptReaderUtils::RGBColourSpecification>(tokens, parser_state);
		}
		else
		{
			return try_process_fbn<CptReaderUtils::HSVColourSpecification>(tokens, parser_state);
		}
	}


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

		// Note the use of the short-circuiting mechanism.
		if (try_process_rgb_or_hsv_colour_slice(tokens, parser_state) ||
				try_process_colour_slice<CptReaderUtils::GMTNameColourSpecification>(tokens, parser_state) ||
				try_process_rgb_or_hsv_fbn(tokens, parser_state) ||
				try_process_colour_slice<CptReaderUtils::CMYKColourSpecification>(tokens, parser_state) ||
				try_process_colour_slice<CptReaderUtils::GreyColourSpecification>(tokens, parser_state) ||
				try_process_colour_slice<CptReaderUtils::InvisibleColourSpecification>(tokens, parser_state) ||
				try_process_colour_slice<CptReaderUtils::PatternFillColourSpecification>(tokens, parser_state))
		{
			parser_state.any_successful_lines = true;
		}
		else
		{
			parser_state.errors.d_recoverable_errors.push_back(
					make_read_error_occurrence(
						parser_state.data_source,
						parser_state.current_line_number,
						ReadErrors::InvalidRegularCptLine,
						ReadErrors::CptLineIgnored));
		}
	}
}


GPlatesGui::RegularCptColourPalette *
GPlatesFileIO::RegularCptReader::read_file(
		QTextStream &text_stream,
		ReadErrorAccumulation &errors,
		boost::shared_ptr<DataSource> data_source) const
{
	GPlatesGui::RegularCptColourPalette *palette = new GPlatesGui::RegularCptColourPalette();
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
}


GPlatesGui::RegularCptColourPalette *
GPlatesFileIO::RegularCptReader::read_file(
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

