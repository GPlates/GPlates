/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
#define GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H

#include <vector>
#include <boost/optional.hpp>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "app-logic/ResolvedRaster.h"

#include "gui/Colour.h"
#include "gui/RasterColourPalette.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"


namespace GPlatesViewOperations
{
	class RenderedResolvedRaster :
			public RenderedGeometryImpl
	{
	public:
		explicit
		RenderedResolvedRaster(
				const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour) :
			d_resolved_raster(resolved_raster),
			d_raster_colour_palette(raster_colour_palette),
			d_raster_modulate_colour(raster_modulate_colour)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_resolved_raster(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// TODO: Implement query into georeferenced raster bounds.
			return GPlatesMaths::ProximityHitDetail::null;
		}

		GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type
		get_resolved_raster() const
		{
			return d_resolved_raster;
		}

		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &
		get_raster_colour_palette() const
		{
			return d_raster_colour_palette;
		}

		const GPlatesGui::Colour &
		get_raster_modulate_colour() const
		{
			return d_raster_modulate_colour;
		}

		/**
		 * Returns the reconstruction time at which raster is resolved/reconstructed.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_resolved_raster->get_reconstruction_time();
		}

	private:
		/**
		 * The resolved raster.
		 */
		GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type d_resolved_raster;

		/**
		 * The colour palette used to colour integral and floating-point rasters.
		 * Note that this colour palette is permitted to be invalid, e.g. for RGBA rasters.
		 */
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_raster_colour_palette;

		/**
		 * The modulation colour to multiply the raster with.
		 */
		GPlatesGui::Colour d_raster_modulate_colour;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
