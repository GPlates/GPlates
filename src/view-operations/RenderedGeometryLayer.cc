/* $Id$ */

/**
 * \file 
 * Used to group a subset of @a RenderedGeometry objects.
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

#include <boost/bind.hpp>

#include "RenderedGeometryLayer.h"
#include "RenderedGeometryLayerVisitor.h"


GPlatesViewOperations::RenderedGeometryLayer::RenderedGeometryLayer(
		user_data_type user_data) :
d_user_data(user_data),
d_is_active(false)
{
}

void
GPlatesViewOperations::RenderedGeometryLayer::set_active(
		bool active)
{
	if (active != d_is_active)
	{
		d_is_active = active;

		emit layer_was_updated(*this, d_user_data);
	}
}

void
GPlatesViewOperations::RenderedGeometryLayer::add_rendered_geometry(
		RenderedGeometry rendered_geom)
{
	d_rendered_geom_seq.push_back(rendered_geom);

	emit layer_was_updated(*this, d_user_data);
}

void
GPlatesViewOperations::RenderedGeometryLayer::clear_rendered_geometries()
{
	if (!d_rendered_geom_seq.empty())
	{
		d_rendered_geom_seq.clear();

		emit layer_was_updated(*this, d_user_data);
	}
}

void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		ConstRenderedGeometryLayerVisitor &visitor) const
{
	// Visit each RenderedGeometry.
	std::for_each(
		d_rendered_geom_seq.begin(),
		d_rendered_geom_seq.end(),
		boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
}

void
GPlatesViewOperations::RenderedGeometryLayer::accept_visitor(
		RenderedGeometryLayerVisitor &visitor)
{
	// Visit each RenderedGeometry.
	std::for_each(
		d_rendered_geom_seq.begin(),
		d_rendered_geom_seq.end(),
		boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(visitor)));
}
