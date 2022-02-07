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

#include "ColourPaletteUtils.h"

#include "ColourPaletteAdapter.h"

#include "file-io/CptReader.h"


GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
		const QString &palette_file_name,
		bool allow_integer_colour_palette,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
{
	if (palette_file_name.isEmpty())
	{
		return RasterColourPalette::create();
	}

	GPlatesFileIO::RegularCptReader regular_cpt_reader;
	GPlatesFileIO::ReadErrorAccumulation regular_errors;
	GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type regular_colour_palette_opt =
			regular_cpt_reader.read_file(palette_file_name, regular_errors);

	// There is a slight complication in the detection of whether a
	// CPT file is regular or categorical. For the most part, a line
	// in a categorical CPT file looks nothing like a line in a
	// regular CPT file and will not be successfully parsed; the
	// exception to the rule are the "BFN" lines, the format of
	// which is common to both regular and categorical CPT files.
	// For that reason, we also check if the regular_palette has any
	// ColourSlices.
	//
	// Note: this flow of code is very similar to that in class IntegerCptReader.
	if (regular_colour_palette_opt)
	{
		// Create a regular colour palette if there are ColourSlice entries.
		if (regular_colour_palette_opt->size())
		{
			// Add the regular errors reported to read_errors.
			read_errors.accumulate(regular_errors);

			const ColourPalette<double>::non_null_ptr_type colour_palette =
					GPlatesGui::convert_colour_palette<GPlatesMaths::Real, double>(
							regular_colour_palette_opt.get(),
							GPlatesGui::RealToBuiltInConverter<double>());

			return RasterColourPalette::create<double>(colour_palette);
		}
		// If there are no entries but the client has requested only real-valued palettes
		// then return the regular colour palette (with no entries) if the CPT file contains only
		// "BFN" lines for some reason - ie, is not a categorical CPT file with entries.
		else if (!allow_integer_colour_palette)
		{
			// Add the regular errors reported to read_errors.
			read_errors.accumulate(regular_errors);

			// Determine if it was in fact a categorical CPT file.
			GPlatesFileIO::CategoricalCptReader<boost::int32_t>::Type categorical_cpt_reader;
			GPlatesFileIO::ReadErrorAccumulation categorical_errors;
			GPlatesGui::CategoricalCptColourPalette<boost::int32_t>::maybe_null_ptr_type categorical_colour_palette_opt =
					categorical_cpt_reader.read_file(palette_file_name, categorical_errors);
			if (categorical_colour_palette_opt &&
				categorical_colour_palette_opt->size())
			{
				// It was a categorical CPT file but we wanted a regular CPT file.
				// So return failure (empty colour palette).
				return RasterColourPalette::create();
			}

			const ColourPalette<double>::non_null_ptr_type colour_palette =
					GPlatesGui::convert_colour_palette<GPlatesMaths::Real, double>(
							regular_colour_palette_opt.get(),
							GPlatesGui::RealToBuiltInConverter<double>());

			// Return the regular colour palette (with no ColourSlice entries).
			return RasterColourPalette::create<double>(colour_palette);
		}
	}

	if (allow_integer_colour_palette)
	{
		// Attempt to read the file as a regular CPT file has failed.
		// Now, let's try to parse it as a categorical CPT file.
		GPlatesFileIO::CategoricalCptReader<boost::int32_t>::Type categorical_cpt_reader;
		GPlatesFileIO::ReadErrorAccumulation categorical_errors;
		GPlatesGui::CategoricalCptColourPalette<boost::int32_t>::maybe_null_ptr_type categorical_colour_palette_opt =
				categorical_cpt_reader.read_file(palette_file_name, categorical_errors);

		if (categorical_colour_palette_opt)
		{
			// If the colour palette just contains "BFN" lines and no ColourEntrys then return the
			// *regular* colour palette (with no ColourSlice entries) if it exists (instead of *categorical*).
			//
			// We do this since a real-valued colour palette is more general than an integer-valued
			// colour palette since the former is suitable for real or integer-valued
			// data whereas the latter is suitable only for integer-valued data.
			if (categorical_colour_palette_opt->size() == 0 &&
				regular_colour_palette_opt)
			{
				// Add the regular errors reported to read_errors.
				read_errors.accumulate(regular_errors);

				const ColourPalette<double>::non_null_ptr_type colour_palette =
						GPlatesGui::convert_colour_palette<GPlatesMaths::Real, double>(
								regular_colour_palette_opt.get(),
								GPlatesGui::RealToBuiltInConverter<double>());

				return RasterColourPalette::create<double>(colour_palette);
			}

			// This time, we return the colour palette even if it just
			// contains "BFN" lines and no ColourEntrys.

			// Add the categorical errors reported to read_errors.
			read_errors.accumulate(categorical_errors);

			const GPlatesGui::ColourPalette<boost::int32_t>::non_null_ptr_type colour_palette(
					categorical_colour_palette_opt.get());

			return RasterColourPalette::create<boost::int32_t>(colour_palette);
		}
	}

	// We failed reading a regular (and a categorical CPT file if we were allowed to try).
	// So we'll just assume it was meant to be a regular CPT file load failure and
	// hence report all the regular errors.
	read_errors.accumulate(regular_errors);

	return RasterColourPalette::create();
}
