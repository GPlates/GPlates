/* $Id$ */

/**
 * \file 
 * Contains functions common to both categorical and regular CPT readers.
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

#ifndef GPLATES_FILEIO_CPTREADERUTILS_H
#define GPLATES_FILEIO_CPTREADERUTILS_H

#include <boost/tuple/tuple.hpp>
#include <QString>
#include <QStringList>

#include "gui/Colour.h"
#include "gui/CptColourPalette.h"


using boost::tuples::get;

namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesFileIO
{
	namespace CptReaderUtils
	{
		struct BadTokenException {  };
		struct BadComponentsException {  };
		struct PatternFillEncounteredException {  };

		template<typename T>
		T
		parse_token(const QString &token);
			// This is intentionally not defined anywhere.

		template<>
		int
		parse_token<int>(const QString &token);

		template<>
		double
		parse_token<double>(const QString &token);

		template<>
		const QString &
		parse_token<const QString &>(const QString &token);

		template<>
		GPlatesGui::ColourScaleAnnotation::Type
		parse_token<GPlatesGui::ColourScaleAnnotation::Type>(const QString &token);

		/**
		 * Parses a series of string tokens, starting from @a starting_index in
		 * @a tokens, into the other types specified by ComponentsType, which is
		 * expected to be a boost::tuple.
		 */
		template<class ComponentsType>
		ComponentsType
		parse_components(
				const QStringList &tokens,
				int starting_index)
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
				int starting_index);

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
				int value);

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
				int h, double s, double v);

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

		GPlatesGui::Colour
		make_gmt_colour(
				const QString &name);

		struct RGBColourSpecification
		{
			typedef boost::tuple<int, int, int> components_type;
			
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

		struct HSVColourSpecification
		{
			typedef boost::tuple<int, double, double> components_type;

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
				if (token.startsWith('p'))
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
					return boost::none;
				}
				else
				{
					throw BadComponentsException();
				}
			}
		};
	}
}

#endif  // GPLATES_FILEIO_CPTREADERUTILS_H
