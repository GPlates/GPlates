/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
#define GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"


namespace GPlatesViewOperations
{
	class RenderedResolvedRaster :
			public RenderedGeometryImpl
	{
	public:
		explicit
		RenderedResolvedRaster(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster) :
		d_georeferencing(georeferencing),
		d_raster(raster)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_resolved_raster(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// TODO: Implement query into georeferenced raster bounds.
			return GPlatesMaths::ProximityHitDetail::null;
		}

		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &
		get_georeferencing() const
		{
			return d_georeferencing;
		}

		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &
		get_raster() const
		{
			return d_raster;
		}

	private:
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;
		GPlatesPropertyValues::RawRaster::non_null_ptr_type d_raster;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDRESOLVEDRASTER_H
