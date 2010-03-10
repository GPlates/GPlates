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

GPlatesGui::MapTransform::MapTransform() :
	d_total_translation_x(0.0),
	d_total_translation_y(0.0),
	d_total_rotation(0.0),
	d_centre_of_viewport(0.0, 0.0)
{
}

void
GPlatesGui::MapTransform::translate_maps(
		qreal dx, qreal dy)
{
	d_total_translation_x += dx;
	d_total_translation_y += dy;
	emit translate(dx, dy);
}

void
GPlatesGui::MapTransform::rotate_maps(
		double angle)
{
	d_total_rotation += angle;
	emit rotate(angle);
}

void
GPlatesGui::MapTransform::reset_rotation()
{
	double old_rotation = d_total_rotation;
	d_total_rotation = 0.0;
	emit rotate(-old_rotation);
}

qreal
GPlatesGui::MapTransform::get_total_translation_x() const
{
	return d_total_translation_x;
}

qreal
GPlatesGui::MapTransform::get_total_translation_y() const
{
	return d_total_translation_y;
}

double
GPlatesGui::MapTransform::get_total_rotation() const
{
	return d_total_rotation;
}

void
GPlatesGui::MapTransform::set_centre_of_viewport(
		const QPointF &centre_of_viewport)
{
	d_centre_of_viewport = centre_of_viewport;
}

const QPointF &
GPlatesGui::MapTransform::get_centre_of_viewport() const
{
	return d_centre_of_viewport;
}
