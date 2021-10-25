/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <utility>
#include <QDebug>

#include "RemappedColourPaletteParameters.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/BuiltinColourPalettes.h"
#include "gui/ColourPaletteRangeRemapper.h"
#include "gui/ColourPaletteUtils.h"


const double GPlatesPresentation::RemappedColourPaletteParameters::DEFAULT_DEVIATION_FROM_MEAN = 2.0;


GPlatesPresentation::RemappedColourPaletteParameters::RemappedColourPaletteParameters(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &default_colour_palette,
		const double &default_deviation_from_mean) :
	d_default_colour_palette_info(
			default_colour_palette,
			GPlatesGui::ColourPaletteUtils::get_range(*default_colour_palette)
					? GPlatesGui::ColourPaletteUtils::get_range(*default_colour_palette).get()
					// Null range (shouldn't get here though)...
					: std::pair<double, double>(0,0)),
	d_deviation_from_mean(default_deviation_from_mean),
	d_unmapped_colour_palette_info(d_default_colour_palette_info),
	d_mapped_colour_palette_info(d_default_colour_palette_info),
	d_is_currently_mapped(false)
{
}


GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
GPlatesPresentation::RemappedColourPaletteParameters::get_colour_palette() const
{
	if (is_palette_range_mapped())
	{
		return d_mapped_colour_palette_info.colour_palette;
	}

	return d_unmapped_colour_palette_info.colour_palette;
}


const std::pair<double, double> &
GPlatesPresentation::RemappedColourPaletteParameters::get_palette_range() const
{
	if (is_palette_range_mapped())
	{
		return d_mapped_colour_palette_info.palette_range;
	}

	return d_unmapped_colour_palette_info.palette_range;
}


void
GPlatesPresentation::RemappedColourPaletteParameters::use_default_colour_palette()
{
	// Should not fail to remap a default colour palette because they should all be
	// of type @a RegularCptColourPalette.
	if (!set_colour_palette(
			QString(), // No filename used for default palettes.
			QString(), // No name used for default palettes.
			boost::none/*builtin_colour_palette_type*/,
			d_default_colour_palette_info.colour_palette,
			d_default_colour_palette_info.palette_range))
	{
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


bool
GPlatesPresentation::RemappedColourPaletteParameters::load_colour_palette(
		const QString &filename,
		GPlatesFileIO::ReadErrorAccumulation &read_errors,
		bool allow_integer_colour_palette)
{
	GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
			GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
					filename,
					allow_integer_colour_palette,
					read_errors);

	if (GPlatesGui::RasterColourPaletteType::get_type(*raster_colour_palette) ==
		GPlatesGui::RasterColourPaletteType::INVALID)
	{
		return false;
	}

	// The palette range.
	boost::optional< std::pair<double, double> > range =
			GPlatesGui::ColourPaletteUtils::get_range(*raster_colour_palette);
	if (!range)
	{
		// Null range for empty colour palette.
		range = std::pair<double, double>(0,0);
	}

	return set_colour_palette(
			filename,
			filename, // Filename used as name for palettes loaded from files.
			boost::none/*builtin_colour_palette_type*/,
			raster_colour_palette,
			range.get());
}


void
GPlatesPresentation::RemappedColourPaletteParameters::load_builtin_colour_palette(
		const GPlatesGui::BuiltinColourPaletteType &builtin_colour_palette_type)
{
	GPlatesGui::RasterColourPalette::non_null_ptr_type raster_colour_palette =
			builtin_colour_palette_type.create_palette();

	// The palette range.
	boost::optional< std::pair<double, double> > range =
			GPlatesGui::ColourPaletteUtils::get_range(*raster_colour_palette);
	if (!range)
	{
		// Null range for empty colour palette.
		range = std::pair<double, double>(0,0);
	}

	// Should not fail to remap a built-in colour palette because they should all be
	// of type @a RegularCptColourPalette.
	if (!set_colour_palette(
			QString(), // No filename used for built-in colour palette types.
			builtin_colour_palette_type.get_palette_name(),
			builtin_colour_palette_type,
			raster_colour_palette,
			range.get()))
	{
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


bool
GPlatesPresentation::RemappedColourPaletteParameters::map_palette_range(
		double lower_bound,
		double upper_bound)
{
	// Make sure not exactly the same...
	//
	// NOTE: Not using 'GPlatesMaths::are_almost_exactly_equal()') since some values
	// may be *very* small and it would compare then equal.
	if (!(lower_bound < upper_bound) &&
		!(lower_bound > upper_bound))
	{
		// Lower and upper bounds are the same.
		// Give them a tiny spread so that the colour palette has a non-zero range.
		lower_bound *= 1 - 1e-6;
		upper_bound *= 1 + 1e-6;

		if (lower_bound > upper_bound)
		{
			std::swap(lower_bound, upper_bound);
		}
	}

	boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> remapped_colour_palette =
			GPlatesGui::remap_colour_palette_range(
					d_unmapped_colour_palette_info.colour_palette,
					lower_bound,
					upper_bound);
	if (!remapped_colour_palette)
	{
		qWarning() << "Failed to map colour palette - using original palette range";
		unmap_palette_range();
		return false;
	}

	d_mapped_colour_palette_info.colour_palette =
			GPlatesGui::RasterColourPalette::create<double>(remapped_colour_palette.get());
	d_mapped_colour_palette_info.palette_range = std::make_pair(lower_bound, upper_bound);

	d_is_currently_mapped = true;

	return true;
}


bool
GPlatesPresentation::RemappedColourPaletteParameters::set_colour_palette(
		const QString &filename,
		const QString &name,
		boost::optional<GPlatesGui::BuiltinColourPaletteType> builtin_colour_palette_type,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
		const std::pair<double, double> &palette_range)
{
	const QString prev_colour_palette_filename = d_colour_palette_filename;
	const QString prev_colour_palette_name = d_colour_palette_name;
	const boost::optional<GPlatesGui::BuiltinColourPaletteType> prev_builtin_colour_palette_type = d_builtin_colour_palette_type;
	const ColourPaletteInfo prev_unmapped_colour_palette_info = d_unmapped_colour_palette_info;

	d_colour_palette_filename = filename;
	d_colour_palette_name = name;
	d_builtin_colour_palette_type = builtin_colour_palette_type;
	d_unmapped_colour_palette_info.colour_palette = colour_palette;
	d_unmapped_colour_palette_info.palette_range = palette_range;

	// If the previous colour palette was mapped then also map the new colour palette.
	if (is_palette_range_mapped())
	{
		if (!map_palette_range(
			d_mapped_colour_palette_info.palette_range.first,
			d_mapped_colour_palette_info.palette_range.second))
		{
			// Failed to map palette range.
			// It is now unmapped so restore the previous palette filename and unmapped info.
			d_colour_palette_filename = prev_colour_palette_filename;
			d_colour_palette_name = prev_colour_palette_name;
			d_builtin_colour_palette_type = prev_builtin_colour_palette_type;
			d_unmapped_colour_palette_info = prev_unmapped_colour_palette_info;

			// Restore the mapping to what it was previously.
			map_palette_range(
					d_mapped_colour_palette_info.palette_range.first,
					d_mapped_colour_palette_info.palette_range.second);

			return false;
		}
	}

	return true;
}
