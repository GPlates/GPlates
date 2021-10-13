/* $Id$ */

/**
 * @file 
 * Contains the implementation of the MapTransform class.
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

#include "MapTransform.h"

#include "ViewportZoom.h"


const double
GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_X = -180.0;

const double
GPlatesGui::MapTransform::MAX_CENTRE_OF_VIEWPORT_X = 180.0;

const double
GPlatesGui::MapTransform::MIN_CENTRE_OF_VIEWPORT_Y = -90.0;

const double
GPlatesGui::MapTransform::MAX_CENTRE_OF_VIEWPORT_Y = 90.0;


GPlatesGui::MapTransform::MapTransform(
		ViewportZoom &viewport_zoom) :
	d_viewport_zoom(viewport_zoom),
	d_centre_of_viewport(0.0, 0.0),
	d_rotation(0)
{
	QObject::connect(
			&viewport_zoom,
			SIGNAL(zoom_changed()),
			this,
			SLOT(handle_zoom_changed()));
}


void
GPlatesGui::MapTransform::set_centre_of_viewport(
		const point_type &centre_of_viewport)
{
	// Disallow centre of viewport that is out of bounds.
	// Note that we don't do clamping; this is because if the map is rotated and
	// precisely one of the x or y are out of bounds, if we clamp, the map will
	// appear to slide at an angle along one of the edges of the rotated map, even
	// if the user pressed up, down, left or right.
	if (centre_of_viewport.x() < MIN_CENTRE_OF_VIEWPORT_X ||
			centre_of_viewport.x() > MAX_CENTRE_OF_VIEWPORT_X ||
			centre_of_viewport.y() < MIN_CENTRE_OF_VIEWPORT_Y ||
			centre_of_viewport.y() > MAX_CENTRE_OF_VIEWPORT_Y)
	{
		return;
	}

	d_centre_of_viewport = centre_of_viewport;
	emit transform_changed(*this);
}


void
GPlatesGui::MapTransform::translate(
		double dx,
		double dy)
{
	set_centre_of_viewport(d_centre_of_viewport + QPointF(dx, dy));
}


void
GPlatesGui::MapTransform::set_rotation(
		double rotation)
{
	d_rotation = rotation;

	// Make sure rotation is between -360 and 360.
	if (d_rotation > 360.0 || d_rotation < -360.0)
	{
		int num_steps = static_cast<int>(d_rotation / 360.0);
		d_rotation -= num_steps * 360.0;
	}

	emit transform_changed(*this);
}


void
GPlatesGui::MapTransform::rotate(
		double angle)
{
	set_rotation(d_rotation + angle);
}


double
GPlatesGui::MapTransform::get_zoom_factor() const
{
	return d_viewport_zoom.zoom_factor();
}


void
GPlatesGui::MapTransform::handle_zoom_changed()
{
	emit transform_changed(*this);
}

