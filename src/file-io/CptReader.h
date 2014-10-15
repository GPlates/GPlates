/* $Id$ */

/**
 * \file 
 * Contains the template class CptReader.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.h")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "CptReader.h")
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

#ifndef GPLATES_FILEIO_CPTREADER_H
#define GPLATES_FILEIO_CPTREADER_H

#include <limits>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "ReadErrorAccumulation.h"
#include "ReadErrorOccurrence.h"

#include "gui/Colour.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"

#include "global/LogException.h"

#include "maths/Real.h"

#include "utils/Parse.h"

// Undefine the min and max macros as they can interfere with the min and
// max functions in std::numeric_limits<T>, on Visual Studio.
#if defined(_MSC_VER)
	#undef min
	#undef max
#endif


namespace GPlatesFileIO
{
	namespace CptReaderInternals
	{
		using boost::tuples::get;
		using GPlatesUtils::Int;


		/**
		 * For use as template parameter to CptReader.
		 */
		struct RegularCptFileFormat
		{
			typedef GPlatesGui::RegularCptColourPalette colour_palette_type;
			typedef double value_type;
		};


		/**
		 * For use as template parameter to CptReader.
		 */
		template<typename T>
		struct CategoricalCptFileFormat
		{
			typedef GPlatesGui::CategoricalCptColourPalette<T> colour_palette_type;
			typedef int value_type;
		};


		struct BadTokenException {  };
		struct BadComponentsException {  };
		struct PatternFillEncounteredException {  };


		template<typename T>
		struct LowestValue;
			// This is not defined anywhere intentionally.


		template<>
		struct LowestValue<double>
		{
			static
			double
			value()
			{
				return GPlatesMaths::negative_infinity<double>();
			}
		};


		template<>
		struct LowestValue<int>
		{
			static
			int
			value()
			{
				return std::numeric_limits<int>::min();
			}
		};


		template<typename T>
		T
		parse_token(const QString &token)
		{
			try
			{
				GPlatesUtils::Parse<T> parse;
				return parse(token);
			}
			catch (const GPlatesUtils::ParseError &)
			{
				throw BadTokenException();
			}
		}


		/**
		 * Parses a series of string tokens, starting from @a starting_index in
		 * @a tokens, into the other types specified by ComponentsType, which is
		 * expected to be a boost::tuple.
		 */
		template<class ComponentsType>
		ComponentsType
		parse_components(
				const QStringList &tokens,
				unsigned int starting_index = 0)
		{
			return boost::tuples::cons<typename ComponentsType::head_type, typename ComponentsType::tail_type>(
					parse_token<typename ComponentsType::head_type>(
						tokens.at(starting_index)),
					parse_components<typename ComponentsType::tail_type>(
						tokens,
						starting_index + 1));
		}


		// Terminating case for recursion.
		template<>
		boost::tuples::null_type
		parse_components<boost::tuples::null_type>(
				const QStringList &tokens,
				unsigned int starting_index);


		/**
		 * Returns true if the @a value lies within the valid range of a "red", "green"
		 * or "blue" token in a CPT file.
		 */
		bool
		in_rgb_range(
				int value);


		/**
		 * Creates a GPlates Colour from the RGB values specified in a CPT file.
		 */
		GPlatesGui::Colour
		make_rgb_colour(
				int r, int g, int b);


		/**
		 * Returns true if the @a value lies within the valid range of a "hue" token in
		 * a CPT file.
		 */
		bool
		in_h_range(
				double value);


		/**
		 * Returns true if the @a value lies within the valid range of a "saturation"
		 * or "value" token in a CPT file.
		 */
		bool
		in_sv_range(
				double value);


		/**
		 * Creates a GPlates Colour from the HSV values specified in a CPT file.
		 */
		GPlatesGui::Colour
		make_hsv_colour(
				double h, double s, double v);


		/**
		 * Returns true if the @a value lies within the valid range of a "cyan",
		 * "magenta", "yellow" or "black" token in a CPT file.
		 */
		bool
		in_cmyk_range(
				int value);


		/**
		 * Creates a GPlates Colour from the CMYK values specified in a CPT file.
		 */
		GPlatesGui::Colour
		make_cmyk_colour(
				int c, int m, int y, int k);


		/**
		 * Returns true if the @a value lies within the valid range of a "grey" token
		 * in a CPT file.
		 */
		bool
		in_grey_range(
				int value);


		/**
		 * Creates a GPlates Colour from the grey value specified in a CPT file.
		 */
		GPlatesGui::Colour
		make_grey_colour(
				int value);


		/**
		 * Creates a GPlates Colour from a GMT colour name.
		 */
		GPlatesGui::Colour
		make_gmt_colour(
				const QString &name);


		/**
		 * Returns true if it is in the format of a pattern fill.
		 */
		bool
		is_pattern_fill_specification(
				const QString &token);


		template<int Base>
		struct BaseRGBColourSpecification
		{
			typedef boost::tuple<Int<Base>, Int<Base>, Int<Base> > components_type;
			
			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				return make_rgb_colour(
					get<0>(components),
					get<1>(components),
					get<2>(components));
			}
		};


		typedef BaseRGBColourSpecification<10> RGBColourSpecification;
		typedef BaseRGBColourSpecification<16> HexRGBColourSpecification;


		struct HSVColourSpecification
		{
			typedef boost::tuple<double, double, double> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type& components)
			{
				return make_hsv_colour(
						get<0>(components),
						get<1>(components),
						get<2>(components));
			}
		};


		struct CMYKColourSpecification
		{
			typedef boost::tuple<int, int, int, int> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				return make_cmyk_colour(
						get<0>(components),
						get<1>(components),
						get<2>(components),
						get<3>(components));
			}
		};


		struct GreyColourSpecification
		{
			typedef boost::tuple<int> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				return make_grey_colour(get<0>(components));
			}
		};


		struct GMTNameColourSpecification
		{
			typedef boost::tuple<const QString &> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				return make_gmt_colour(get<0>(components));
			}
		};


		struct PatternFillColourSpecification
		{
			typedef boost::tuple<const QString &> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				const QString &token = get<0>(components);
				if (is_pattern_fill_specification(token))
				{
					// We do not support pattern fills. Testiing for the first character only
					// isn't entirely right (we don't validate the rest of the pattern fill).
					throw PatternFillEncounteredException();
				}
				else
				{
					throw BadComponentsException();
				}
			}
		};


		struct InvisibleColourSpecification
		{
			typedef boost::tuple<const QString &> components_type;

			static
			inline
			boost::optional<GPlatesGui::Colour>
			convert(
					const components_type &components)
			{
				const QString &token = get<0>(components);
				if (token == "-")
				{
					// Slices specified with a dash are not drawn.
					return boost::none;
				}
				else
				{
					throw BadComponentsException();
				}
			}
		};


		/**
		 * Parses components and converts the parsed components into a colour.
		 */
		template<class ColourSpecification>
		inline
		boost::optional<GPlatesGui::Colour>
		convert_tokens(
				const QStringList &tokens,
				unsigned int starting_index = 0)
		{
			return ColourSpecification::convert(
					parse_components<typename ColourSpecification::components_type>(tokens, starting_index));
		}


		/**
		 * Stores the state of the CPT parser as it proceeds through the file.
		 */
		template<class CptFileFormat>
		struct ParserState :
				public boost::noncopyable
		{
			typedef typename CptFileFormat::colour_palette_type colour_palette_type;
			typedef typename CptFileFormat::value_type value_type;

			ParserState(
					colour_palette_type &palette_,
					ReadErrorAccumulation &errors_,
					boost::shared_ptr<DataSource> data_source_) :
				palette(palette_),
				errors(errors_),
				data_source(data_source_),
				rgb(true),
				any_successful_lines(false),
				error_reported_for_current_line(false),
				current_line_number(0),
				previous_upper_value(LowestValue<value_type>::value())
			{
			}

			/**
			 * The data structure that holds all lines successfully read in.
			 */
			colour_palette_type &palette;

			/**
			 * For the reporting of read errors.
			 */
			ReadErrorAccumulation &errors;

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
			 * True if an error has already been reported for the current line; used to
			 * prevent cascading errors being reported.
			 */
			bool error_reported_for_current_line;

			/**
			 * The line number that we're currently parsing.
			 */
			unsigned long current_line_number;

			/**
			 * Stores the upper z-value of the previous slice.
			 */
			value_type previous_upper_value;
		};


		/**
		 * Attempts to process a line in a regular or categorical CPT file as a comment.
		 *
		 * This function also checks if the comment is a special comment that switches
		 * the colour model to RGB or HSV.
		 *
		 * Returns true if successful. Note that this function does not set the
		 * any_successful_lines variable in @a parser_state because the notion of
		 * successful lines only includes successful non-comment lines.
		 */
		template<class CptFileFormat>
		bool
		try_process_comment(
				const QString &line,
				ParserState<CptFileFormat> &parser_state)
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
		 * Attempts to process a line in a regular or categorical CPT file as a "BFN" line.
		 *
		 * The format of the line is one of:
		 *
		 *		B	R	G	B
		 *		F	R	G	B
		 *		N	R	G	B
		 * 
		 * if the ColourSpecification parameter is CptReaderInternals::RGBColourSpecification.
		 * The only other valid ColourSpecification for a BFN line is
		 * CptReaderInternals::HSVColourSpecification; in that case, the R, G and B
		 * components are replaced by H, S and V components respectively.
		 *
		 * If the line was successfully parsed, the function returns true and changes
		 * the foreground, background or NaN colours in the colour palette as appropriate.
		 */
		template<class CptFileFormat, class ColourSpecification>
		bool
		try_process_bfn(
				const QStringList &tokens,
				ParserState<CptFileFormat> &parser_state)
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
				components_type colour_components = parse_components<components_type>(tokens, 1);
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
		template<class CptFileFormat>
		bool
		try_process_rgb_or_hsv_bfn(
				const QStringList &tokens,
				ParserState<CptFileFormat> &parser_state)
		{
			if (parser_state.rgb)
			{
				return try_process_bfn<CptFileFormat, CptReaderInternals::RGBColourSpecification>(tokens, parser_state);
			}
			else
			{
				return try_process_bfn<CptFileFormat, CptReaderInternals::HSVColourSpecification>(tokens, parser_state);
			}
		}


		/**
		 * Parses the optional label at the end of a regular CPT file line. The label
		 * starts with a semi-colon.
		 */
		boost::optional<QString>
		parse_regular_cpt_label(
				const QString &token);


		/**
		 * Attempts to process a regular CPT file line as a colour slice.
		 *
		 * The format of the line is:
		 *
		 *		lower_value R G B upper_value R G B [a] [;label]
		 *
		 * if the ColourSpecification parameter is CptReaderInternals::RGBColourSpecification.
		 * For other choices of ColourSpecification, R, G and B are replaced as appropriate.
		 *
		 * If the line was successfully parsed, the function returns true and inserts a
		 * new entry into the current colour palette.
		 */
		template<class ColourSpecification>
		bool
		try_process_regular_cpt_colour_slice(
				const QStringList &tokens,
				ParserState<RegularCptFileFormat> &parser_state)
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
				double lower_value = CptReaderInternals::parse_token<double>(tokens.at(0));

				// First lot of colour components.
				boost::optional<GPlatesGui::Colour> lower_colour = convert_tokens<ColourSpecification>(tokens, 1);

				// Upper value of z-slice.
				double upper_value = CptReaderInternals::parse_token<double>(tokens.at(1 + NUM_COMPONENTS));

				// Second lot of colour components.
				boost::optional<GPlatesGui::Colour> upper_colour = convert_tokens<ColourSpecification>(tokens, 1 + NUM_COMPONENTS + 1);

				// Parse the last 2 tokens, if any.
				GPlatesGui::ColourScaleAnnotation::Type annotation = (tokens.count() == MIN_TOKENS_COUNT) ?
					GPlatesGui::ColourScaleAnnotation::NONE :
					CptReaderInternals::parse_token<GPlatesGui::ColourScaleAnnotation::Type>(tokens.at(MIN_TOKENS_COUNT));
				boost::optional<QString> label = (tokens.count() == MAX_TOKENS_COUNT) ?
					parse_regular_cpt_label(tokens.at(MAX_TOKENS_COUNT - 1)) :
					boost::none;
				
				// Issue a warning if this slice does not start after the end of the previous slice.
				if (GPlatesMaths::Real(lower_value) < GPlatesMaths::Real(parser_state.previous_upper_value))
				{
					parser_state.errors.d_warnings.push_back(
							make_read_error_occurrence(
								parser_state.data_source,
								parser_state.current_line_number,
								ReadErrors::CptSliceNotMonotonicallyIncreasing,
								ReadErrors::NoAction));
				}
				parser_state.previous_upper_value = upper_value;

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
			catch (const CptReaderInternals::PatternFillEncounteredException &)
			{
				parser_state.errors.d_recoverable_errors.push_back(
						make_read_error_occurrence(
							parser_state.data_source,
							parser_state.current_line_number,
							ReadErrors::PatternFillInLine,
							ReadErrors::CptLineIgnored));
				parser_state.error_reported_for_current_line = true;

				return false;
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
		try_process_regular_cpt_rgb_or_hsv_colour_slice(
				const QStringList &tokens,
				ParserState<RegularCptFileFormat> &parser_state);


		/**
		 * Attempts to parse the fill specification on a categorical CPT line.
		 */
		boost::optional<GPlatesGui::Colour>
		parse_categorical_fill(
				const QString &token);


		/**
		 * Attempts to process a line as an entry in a categorical CPT file.
		 *
		 * The line is of the format:
		 *
		 *		key fill label
		 *
		 * where the key must be greater than the previous key and the label is optional.
		 * The fill is in a format specified in section 4.14 of the GMT docs
		 * (http://www.soest.hawaii.edu/gmt/gmt/doc/gmt/html/GMT_Docs/node66.html).
		 *
		 * If the line was successfully parsed, the function returns true and inserts a
		 * new entry into the current colour palette.
		 */
		template<typename T>
		bool
		try_process_categorical_cpt_colour_entry(
				const QStringList &tokens,
				ParserState<CategoricalCptFileFormat<T> > &parser_state)
		{
			static const bool is_label_optional = GPlatesGui::ColourEntry<T>::is_label_optional;

			// Must have two or three tokens.
			if (!(tokens.count() == 3 ||
						(is_label_optional && tokens.count() == 2)))
			{
				return false;
			}

			try
			{
				// Parse the key.
				int key = parse_token<int>(tokens.at(0));

				// Parse the fill.
				boost::optional<GPlatesGui::Colour> colour = parse_categorical_fill(tokens.at(1));
				if (!colour)
				{
					return false;
				}

				// Get the label, if it exists.
				boost::optional<QString> label = tokens.count() == 3 ?
					boost::optional<QString>(tokens.at(2)) : boost::none;

				// Send everything off to be created.
				parser_state.palette.add_entry(
						GPlatesGui::make_colour_entry<T>(key, *colour, label));

				// Check that this line's key is after the previous line's key.
				if (key < parser_state.previous_upper_value)
				{
					parser_state.errors.d_warnings.push_back(
							make_read_error_occurrence(
								parser_state.data_source,
								parser_state.current_line_number,
								ReadErrors::CptSliceNotMonotonicallyIncreasing,
								ReadErrors::NoAction));
				}
				parser_state.previous_upper_value = key;

				return true;
			}
			catch (const PatternFillEncounteredException &)
			{
				parser_state.errors.d_recoverable_errors.push_back(
						make_read_error_occurrence(
							parser_state.data_source,
							parser_state.current_line_number,
							ReadErrors::PatternFillInLine,
							ReadErrors::CptLineIgnored));
				parser_state.error_reported_for_current_line = true;

				return false;
			}
			catch (const GPlatesUtils::ParseError &)
			{
				parser_state.errors.d_recoverable_errors.push_back(
						make_read_error_occurrence(
							parser_state.data_source,
							parser_state.current_line_number,
							ReadErrors::UnrecognisedLabel,
							ReadErrors::CptLineIgnored));
				parser_state.error_reported_for_current_line = true;

				return false;
			}
			catch (...)
			{
				return false;
			}
		}


		template<class CptFileFormat>
		struct TryProcessTokensImpl;
			// This is intentionally not defined anywhere.


		template<>
		struct TryProcessTokensImpl<RegularCptFileFormat>
		{
			bool
			operator()(
					const QStringList &tokens,
					ParserState<RegularCptFileFormat> &parser_state);
		};


		template<typename T>
		struct TryProcessTokensImpl<CategoricalCptFileFormat<T> >
		{
			bool
			operator()(
					const QStringList &tokens,
					ParserState<CategoricalCptFileFormat<T> > &parser_state)
			{
				// Note the use of the short-circuiting mechanism.
				return try_process_categorical_cpt_colour_entry(tokens, parser_state) ||
						try_process_rgb_or_hsv_bfn<CategoricalCptFileFormat<T> >(tokens, parser_state);
			}
		};


		template<class CptFileFormat>
		struct TryProcessLineImpl
		{
			void
			operator()(
					const QString &line,
					ParserState<CptFileFormat> &parser_state)
			{
				if (try_process_comment(line, parser_state))
				{
					return;
				}

				// Split the string by whitespace.
				QStringList tokens = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

				// Note the use of the short-circuiting mechanism.
				TryProcessTokensImpl<CptFileFormat> try_process_tokens_impl;
				if (try_process_tokens_impl(tokens, parser_state))
				{
					parser_state.any_successful_lines = true;
				}
				else if (!parser_state.error_reported_for_current_line)
				{
					parser_state.errors.d_recoverable_errors.push_back(
							make_read_error_occurrence(
								parser_state.data_source,
								parser_state.current_line_number,
								ReadErrors::InvalidRegularCptLine,
								ReadErrors::CptLineIgnored));
				}
				parser_state.error_reported_for_current_line = false;
			}


			bool
			try_process_tokens(
					const QStringList &tokens,
					ParserState<CptFileFormat> &parser_state);
		};


		/**
		 * Attempts to parse a line in a CPT file.
		 *
		 * @a parser_state.any_successful_line is set to true if the @a line was
		 * successfully parsed as a non-comment line.
		 */
		template<class CptFileFormat>
		void
		try_process_line(
				const QString &line,
				ParserState<CptFileFormat> &parser_state)
		{
			TryProcessLineImpl<CptFileFormat> try_process_line_impl;
			try_process_line_impl(line, parser_state);
		}
	}

	/**
	 * This reads in GMT colour palette table (CPT) files.
	 *
	 * If the template parameter CptFileFormat is RegularCptFileFormat, it will
	 * read a "regular" CPT file, which consists of a series of continuous ranges
	 * with colours linearly interpolated over the extent of those ranges.
	 *
	 * If the template parameter CptFileFormat is ContinuousCptFileFormat, it will
	 * read a "categorical" CPT file, which consists of mappings of discrete values
	 * to colours; this variety is used for data types where it makes no sense to
	 * interpolate between values.
	 *
	 * A description of a "regular" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node69.html 
	 *
	 * A description of a "categorical" CPT file can be found at
	 * http://www.soest.hawaii.edu/gmt/gmt/doc/gmt/html/GMT_Docs/node68.html
	 *
	 * This reader does not understand pattern fills.
	 *
	 * This reader also does not respect the .gmtdefaults4 settings file.
	 */
	template<class CptFileFormat>
	class CptReader
	{
	public:

		typedef typename CptFileFormat::colour_palette_type colour_palette_type;

		/**
		 * Parses text from the provided @a text_stream as a regular CPT file.
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
		typename colour_palette_type::maybe_null_ptr_type
		read_file(
				QTextStream &text_stream,
				ReadErrorAccumulation &errors,
				boost::shared_ptr<DataSource> data_source =
					boost::shared_ptr<DataSource>(
						new GenericDataSource(
							DataFormats::Cpt,
							"QTextStream"))) const
		{
			typename colour_palette_type::maybe_null_ptr_type palette = colour_palette_type::create().get();
			CptReaderInternals::ParserState<CptFileFormat> parser_state(*palette, errors, data_source);

			// Go through each line one by one.
			while (!text_stream.atEnd())
			{
				++parser_state.current_line_number;
				QString line = text_stream.readLine().trimmed();

				if (!line.isEmpty())
				{
					CptReaderInternals::try_process_line<CptFileFormat>(line, parser_state);
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

				return typename colour_palette_type::maybe_null_ptr_type();
			}
		}

		/**
		 * A convenience function for reading the file with the given @a filename as
		 * a regular CPT file.
		 *
		 * @see read_file() that takes a QTextStream as the first parameter.
		 */
		typename colour_palette_type::maybe_null_ptr_type
		read_file(
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
				return typename colour_palette_type::maybe_null_ptr_type();
			}
		}
	};

	/**
	 * A file reader that reads a regular CPT file and produces a colour palette.
	 */
	typedef CptReader<CptReaderInternals::RegularCptFileFormat> RegularCptReader;

	// Can't wait for C++0x...
	template<typename T>
	struct CategoricalCptReader
	{
		/**
		 * A file reader that reads a categorical CPT file and produces a colour palette
		 * that maps values of template type T to colours.
		 */
		typedef CptReader<CptReaderInternals::CategoricalCptFileFormat<T> > Type;
	};

	/**
	 * IntegerCptReader parses a file that is either a regular or categorical CPT
	 * file, but it is not known which of the two formats it is actually in.
	 *
	 * It will attempt to parse the file as a regular CPT file first, and if that
	 * succeeds, it will return the (real-valued) colour palette with a wrapper
	 * that converts it into an integer-valued colour palette.
	 *
	 * If the attempt to parse the file as a regular CPT file fails, it will then
	 * attempt to parse it as a categorical CPT file that maps integers to colours.
	 * If that succeeds, it will return the resulting integer-valued colour palette.
	 *
	 * This integrated CPT reader is useful for reading in a CPT file for say,
	 * plate ID. Although plate IDs are discrete values and it is meaningless to
	 * interpolate between plate IDs, it should be possible for users to provide
	 * either a categorical CPT file (where they explicitly map plate IDs to
	 * colours) or a regular CPT file (where they map ranges of plate IDs to
	 * colours).
	 *
	 * Notice that it makes no sense to provide an integrated CPT reader for types
	 * other than int. It doesn't really make sense to map a real-valued property
	 * using a categorical CPT file, and non-numerical types cannot be represented
	 * in a regular CPT file.
	 */
	template<typename IntType = int>
	class IntegerCptReader
	{
	public:

		typedef GPlatesGui::ColourPalette<IntType> colour_palette_type;

		typename GPlatesGui::ColourPalette<IntType>::maybe_null_ptr_type
		read_file(
				const QString &filename,
				ReadErrorAccumulation &errors) const
		{
			// First, attempt to parse as a regular CPT file.
			// We use a temporary ReadErrorAccumulation, just in case the read totally
			// fails and we're not interested in reporting the regular CPT errors.
			RegularCptReader regular_reader;
			ReadErrorAccumulation regular_errors;
			GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type regular_palette =
				regular_reader.read_file(filename, regular_errors);

			if (regular_palette)
			{
				// There is a slight complication in the detection of whether a
				// CPT file is regular or categorical. For the most part, a line
				// in a categorical CPT file looks nothing like a line in a
				// regular CPT file and will not be successfully parsed; the
				// exception to the rule are the "BFN" lines, the format of
				// which is common to both regular and categorical CPT files.
				// For that reason, we also check if the regular_palette has any
				// ColourSlices.
				if (regular_palette->size())
				{
					// Add all the errors reported to errors.
					errors.accumulate(regular_errors);

					// Adapt the palette to accept integers instead.
					typedef GPlatesGui::ColourPaletteAdapter
					<
						GPlatesMaths::Real,
						IntType,
						GPlatesGui::RealToBuiltInConverter<IntType>
					> adapted_type;
					return adapted_type::create(regular_palette).get();
				}
			}

			// Attempt to read the file as a regular CPT file has failed.
			// Now, let's try to parse it as a categorical CPT file.
			typename CategoricalCptReader<IntType>::Type categorical_reader;
			ReadErrorAccumulation categorical_errors;
			typename GPlatesGui::CategoricalCptColourPalette<IntType>::maybe_null_ptr_type categorical_palette =
				categorical_reader.read_file(filename, categorical_errors);

			if (categorical_palette)
			{
				// This time, we return the colour palette even if it just
				// contains "BFN" lines and no ColourEntrys.

				// Add all the errors reported to errors.
				errors.accumulate(categorical_errors);

				return typename GPlatesGui::ColourPalette<IntType>::maybe_null_ptr_type(categorical_palette);
			}

			// We can't make heads or tails of this file.
			errors.d_failures_to_begin.push_back(
					make_read_error_occurrence(
							filename,
							DataFormats::Cpt,
							0,
							ReadErrors::CptFileTypeNotDeduced,
							ReadErrors::FileNotLoaded));

			return typename GPlatesGui::ColourPalette<IntType>::maybe_null_ptr_type();
		}
	};

	class CptParser
	{
		public:
		enum Model
		{
			RGB,
			HSV,
			CMYK,
			RGB_HEX, //#00ff00 
			GREY,
			GMT_NAME,
			EMPTY
		};

		struct ColourData
		{
			ColourData():
				model(GMT_NAME), str_data("black"){}

			Model model;
			std::vector<float> float_array;
			QString str_data;
		};

		struct CategoricalEntry
		{
			QString key;
			ColourData data;
			QString label;
		};


		struct RegularEntry
		{
			float key1, key2;
			ColourData data1, data2;
			QString label_opt, label;
		};

		explicit
		CptParser(const QString& file_path) ;

		std::vector<ColourData>
		bfn_data() const
		{
			std::vector<ColourData> ret;
			ret.push_back(d_back);
			ret.push_back(d_fore);
			ret.push_back(d_nan);
			return ret;
		}

		const std::vector<CategoricalEntry>&
		catagorical_entries()
		{
			return	d_categorical_entries;
		}

		const std::vector<RegularEntry>&
		regular_entries()
		{
			return d_regular_entries;
		}

	protected:
		void
		process_line(
				const QString& line);

		/*
		* Process background color, foreground color and NaN color definition.
		*/
		void
		process_bfn(
				QStringList& tokens,
				ColourData& data);

		/*
		* Process regular cpt data.
		*/
		void
		process_regular_line(
				QStringList& tokens);
		

		ColourData
		read_first_colour_data(
				QStringList& tokens);
		

		ColourData
		read_second_colour_data(
				QStringList& tokens);

		/*
		* Process categorical cpt data.
		*/
		void
		process_categorical_line(		
				QStringList& tokens);

		void
		process_comment(
				const QString& line);

		/*
		* Parse a line of cpt file according to GMT cpt specification..
		*/

		QStringList
		split_into_tokens(
				const QString& line);
		/*
		Fill examples: 
			-G128				Solid gray
			-G127/255/0			Chartreuse, R/G/B-style
			-G#00ff00			Green, hexadecimal RGB code
			-G25-0.86-0.82		Chocolate, h-s-v – style
			-GDarkOliveGreen1	One of the named colors
			-Gp300/7			Simple diagonal hachure pattern in b/w at 300 dpi
			-Gp300/7:Bred		Same, but with red lines on white
			-Gp300/7:BredF-		Now the gaps between red lines are transparent
			-Gp100/marble.ras	Using user image of marble as the fill at 100 dpi
		
		For "fill" specification, see 
			chapter 4.14
			<The Generic Mapping Tools>
			Version 4.5.7
			Technical Reference and Cookbook by Pål (Paul)Wessel
		*/
		ColourData 
		parse_gmt_fill(
				const QString& token);

		bool
		is_gmt_color_name(
				const QString& name);

		bool
		is_valid_rgb(
				float r, 
				float g, 
				float b)
		{
			return !(r<0.0 || r>255.0) && !(g<0.0 || g>255.0) && !(b<0.0 || b>255.0);
		}

		bool
		is_valid_hsv(
				float h, 
				float s, 
				float v)
		{
			return !(h<0.0 || h>360.0) && !(s<0.0 || s>1.0) && !(v<0.0 || v>1.0);
		}

		bool
		is_valid_cmyk(
				float c, 
				float m, 
				float y, 
				float k)
		{
			return !(c<0.0 || c>100.0) && !(m<0.0 || m>100.0) && !(y<0.0 || y>100.0) && !(k<0.0 || k>100.0);
		}

		/*
		* Given the raw data in QStringList, parse the data into ColourData.
		* This function assume the first 3 QString items are color data.
		* This function will strip off the first 3 QString items after parsing it.
		* Caller is responsible for giving expected data input.
		*/
		void
		parse_rbg_data(
				QStringList& tokens, 
				ColourData& data);
		/*
		* Given the raw data in QStringList, parse the data into ColourData.
		* This function assume the first 3 QString items are color data.
		* This function will strip off the first 3 QString items after parsing it.
		* Caller is responsible for giving expected data input.
		*/
		void
		parse_hsv_data(
				QStringList& tokens, 
				ColourData& data);
		/*
		* Given the raw data in QStringList, parse the data into ColourData.
		* This function assume the first 4 QString items are color data.
		* This function will strip off the first 4 QString items after parsing it.
		* Caller is responsible for giving expected data input.
		*/
		void
		parse_cmyk_data(
				QStringList& tokens, 
				ColourData& data);
		

		Model d_default_model;
		ColourData d_back, d_fore, d_nan;
		std::vector<CategoricalEntry> d_categorical_entries;
		std::vector<RegularEntry> d_regular_entries;
	};
}

#endif  // GPLATES_FILEIO_CPTREADER_H
