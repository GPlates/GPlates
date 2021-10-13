/* $Id$ */

/**
 * @file 
 * Contains the definition of the DefaultAgeColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_AGECOLOURPALETTES_H
#define GPLATES_GUI_AGECOLOURPALETTES_H

#include "Colour.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"


namespace GPlatesMaths
{
	class Real;
}

namespace GPlatesGui
{
	/**
	 * Abstract base class for colour palettes that colour by age. The youngest and
	 * oldest ages coloured by the colour palette can be changed dynamically.
	 */
	class AgeColourPalette :
			public ColourPalette<GPlatesMaths::Real>
	{
	public:

		AgeColourPalette(
				const double default_upper_bound,
				const double default_lower_bound) :
			d_upper_bound(default_upper_bound),
			d_lower_bound(default_lower_bound),
			d_default_upper_bound(default_upper_bound),
			d_default_lower_bound(default_lower_bound)
		{ }

		void
		reset_bounds()
		{
			d_upper_bound = d_default_upper_bound;
			d_lower_bound = d_default_lower_bound;
		}

		double
		get_upper_bound() const
		{
			return d_upper_bound;
		}

		void
		set_upper_bound(
				double upper_bound_)
		{
			d_upper_bound = upper_bound_;
		}

		double
		get_lower_bound() const
		{
			return d_lower_bound;
		}

		void
		set_lower_bound(
				double lower_bound_)
		{
			d_lower_bound = lower_bound_;
		}

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_age_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_age_colour_palette(*this);
		}

		/**
		 * Returns the colour for ages younger than the lower bound.
		 * (The terminology "background" comes from CPT files.)
		 */
		virtual
		Colour
		get_background_colour() const = 0;

		/**
		 * Returns the colour for ages older than the upper bound.
		 * (The terminology "foreground" comes from CPT files.)
		 */
		virtual
		Colour
		get_foreground_colour() const = 0;

	protected:

		double d_upper_bound, d_lower_bound;

	private:

		const double d_default_upper_bound, d_default_lower_bound;
	};


	/**
	 * DefaultAgeColourPalette maps age to colours using a rainbow of colours.
	 */
	class DefaultAgeColourPalette :
			public AgeColourPalette
	{
	public:

		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesMaths::Real &geo_time) const;

		virtual
		Colour
		get_background_colour() const;

		virtual
		Colour
		get_foreground_colour() const;
	
	private:
		
		DefaultAgeColourPalette();

		static const double DEFAULT_UPPER_BOUND;
		static const double DEFAULT_LOWER_BOUND;
	};


	/**
	 * MonochromeAgeColourPalette maps age to colours using shades of grey.
	 */
	class MonochromeAgeColourPalette :
			public AgeColourPalette
	{
	public:

		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesMaths::Real &geo_time) const;

		virtual
		Colour
		get_background_colour() const;

		virtual
		Colour
		get_foreground_colour() const;

	private:

		MonochromeAgeColourPalette();

		static const double DEFAULT_UPPER_BOUND;
		static const double DEFAULT_LOWER_BOUND;

		static const Colour UPPER_COLOUR;
		static const Colour LOWER_COLOUR;
	};
}

#endif  /* GPLATES_GUI_AGECOLOURPALETTES_H */
