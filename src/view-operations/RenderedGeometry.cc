/* $Id$ */

/**
 * \file 
 * Pimpl interface class for all rendered geometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "RenderedGeometry.h"
#include "RenderedGeometryImpl.h"
#include "maths/ProximityCriteria.h"


GPlatesViewOperations::RenderedGeometry::RenderedGeometry(
		impl_ptr_type impl) :
d_impl(impl)
{
}

void
GPlatesViewOperations::RenderedGeometry::accept_visitor(
		ConstRenderedGeometryVisitor& rendered_geometry_visitor) const
{
	// If we have an implementation then visit it.
	if (d_impl)
	{
		d_impl->accept_visitor(rendered_geometry_visitor);
	}
}

GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesViewOperations::RenderedGeometry::test_proximity(
		const GPlatesMaths::ProximityCriteria &criteria) const
{
	// If we have an implementation then visit it.
	if (d_impl)
	{
		return d_impl->test_proximity(criteria);
	}

	return GPlatesMaths::ProximityHitDetail::null;
}

GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesViewOperations::RenderedGeometry::test_vertex_proximity(
	const GPlatesMaths::ProximityCriteria &criteria) const
{
	// If we have an implementation then visit it.
	if (d_impl)
	{
		return d_impl->test_vertex_proximity(criteria);
	}

	return GPlatesMaths::ProximityHitDetail::null;
}

