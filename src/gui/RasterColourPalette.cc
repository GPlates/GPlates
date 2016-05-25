/* $Id$ */

/**
 * @file 
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

#include "RasterColourPalette.h"


namespace
{
	/**
	 * A visitor that, in turn, visits a ConstColourPaletteVisitor or ColourPaletteVisitor.
	 */
	template <class ColourPaletteVisitorType>
	class VisitColourPaletteVisitor :
			public boost::static_visitor<>
	{
	public:

		explicit
		VisitColourPaletteVisitor(
				ColourPaletteVisitorType &colour_palette_visitor) :
			d_colour_palette_visitor(colour_palette_visitor)
		{  }

		void
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			// Nothing to visit.
		}

		template <typename ColourPalettePtrType>
		void
		operator()(
				const ColourPalettePtrType &colour_palette_ptr) const
		{
			colour_palette_ptr->accept_visitor(d_colour_palette_visitor);
		}

	private:

		ColourPaletteVisitorType &d_colour_palette_visitor;
	};


	class RasterColourPaletteTypeVisitor :
			public boost::static_visitor<GPlatesGui::RasterColourPaletteType::Type>
	{
	public:

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			return GPlatesGui::RasterColourPaletteType::INVALID;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<boost::int32_t>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::INT32;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<boost::uint32_t>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::UINT32;
		}

		GPlatesGui::RasterColourPaletteType::Type
		operator()(
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &) const
		{
			return GPlatesGui::RasterColourPaletteType::DOUBLE;
		}
	};
}


void
GPlatesGui::RasterColourPalette::accept_visitor(
		ConstColourPaletteVisitor &colour_palette_visitor) const
{
	VisitColourPaletteVisitor<ConstColourPaletteVisitor> visitor(colour_palette_visitor);
	boost::apply_visitor(visitor, d_colour_palette);
}


void
GPlatesGui::RasterColourPalette::accept_visitor(
		ColourPaletteVisitor &colour_palette_visitor) const
{
	VisitColourPaletteVisitor<ColourPaletteVisitor> visitor(colour_palette_visitor);
	boost::apply_visitor(visitor, d_colour_palette);
}


GPlatesGui::RasterColourPaletteType::Type
GPlatesGui::RasterColourPaletteType::get_type(
		const RasterColourPalette &raster_colour_palette)
{
	return boost::apply_visitor(RasterColourPaletteTypeVisitor(), raster_colour_palette);
}
