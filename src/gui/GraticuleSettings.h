/* $Id$ */

/**
 * @file 
 * Holds the user's settings related to graticules on the globe and map.
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

#ifndef GPLATES_GUI_GRATICULESETTINGS_H
#define GPLATES_GUI_GRATICULESETTINGS_H

#include <boost/operators.hpp>

#include "Colour.h"

#include "maths/MathsUtils.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesGui
{
	class GraticuleSettings :
			public boost::equality_comparable<GraticuleSettings>
	{
	public:

		// Default graticule spacing in latitude and longitude.
		static const double DEFAULT_GRATICULE_DELTA_LAT;
		static const double DEFAULT_GRATICULE_DELTA_LON;
		// Default graticule colour.
		static const GPlatesGui::Colour DEFAULT_GRATICULE_COLOUR;


		/**
		 * Constructs a GraticuleSettings. Lines of latitude are rendered @a delta_lat
		 * radians apart, and lines of longitude are rendered @a delta_lon radians
		 * apart. If @a delta_lat or @a delta_lon is zero, lines of latitude or lines
		 * of longitude are not rendered, respectively.
		 */
		GraticuleSettings(
				double delta_lat = DEFAULT_GRATICULE_DELTA_LAT,
				double delta_lon = DEFAULT_GRATICULE_DELTA_LON,
				const GPlatesGui::Colour &colour = DEFAULT_GRATICULE_COLOUR) :
			d_delta_lat(delta_lat),
			d_delta_lon(delta_lon),
			d_colour(colour),
			d_line_width_hint(1.0f)
		{  }

		double
		get_delta_lat() const
		{
			return d_delta_lat;
		}

		void
		set_delta_lat(
				double delta_lat)
		{
			d_delta_lat = delta_lat;
		}

		double
		get_delta_lon() const
		{
			return d_delta_lon;
		}

		void
		set_delta_lon(
				double delta_lon)
		{
			d_delta_lon = delta_lon;
		}

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const GPlatesGui::Colour &colour)
		{
			d_colour = colour;
		}

		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}

		void
		set_line_width_hint(
				float line_width_hint)
		{
			d_line_width_hint = line_width_hint;
		}

	private:

		double d_delta_lat;
		double d_delta_lon;
		GPlatesGui::Colour d_colour;
		float d_line_width_hint;

		friend
		bool
		operator==(
				const GraticuleSettings &lhs,
				const GraticuleSettings &rhs)
		{
			return GPlatesMaths::are_almost_exactly_equal(lhs.d_delta_lat, rhs.d_delta_lat) &&
				GPlatesMaths::are_almost_exactly_equal(lhs.d_delta_lon, rhs.d_delta_lon) &&
				lhs.d_colour == rhs.d_colour &&
				GPlatesMaths::are_almost_exactly_equal(lhs.d_line_width_hint, rhs.d_line_width_hint);
		}

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};

}

#endif  // GPLATES_GUI_GRATICULESETTINGS_H
