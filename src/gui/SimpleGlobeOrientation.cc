/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include "SimpleGlobeOrientation.h"


GPlatesMaths::UnitVector3D
GPlatesGui::SimpleGlobeOrientation::rotation_axis() const {

	return m_accum_rot.axis();
}


GPlatesMaths::real_t
GPlatesGui::SimpleGlobeOrientation::rotation_angle() const {

	return m_accum_rot.angle();
}


GPlatesMaths::PointOnSphere
GPlatesGui::SimpleGlobeOrientation::reverse_orient_point(
 const GPlatesMaths::PointOnSphere &pos) const {

	return (m_rev_accum_rot * pos);
}


void
GPlatesGui::SimpleGlobeOrientation::set_new_handle_at_pos(
 const GPlatesMaths::PointOnSphere &pos) {

	m_handle_pos = pos;
}


void
GPlatesGui::SimpleGlobeOrientation::move_handle_to_pos(
 const GPlatesMaths::PointOnSphere &pos) {

	GPlatesMaths::Rotation rot =
	 GPlatesMaths::CreateRotation(m_handle_pos, pos);

	m_accum_rot = rot * m_accum_rot;
	m_rev_accum_rot = m_accum_rot.reverse();

	m_handle_pos = pos;
}
