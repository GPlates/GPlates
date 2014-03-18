/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "SceneLightingParameters.h"


GPlatesGui::SceneLightingParameters::SceneLightingParameters() :
	d_light_direction_attached_to_view_frame(true),
	// Default to 40% ambient (non-lit) and 60% diffuse lighting since
	// it gives good visual contrast/results for the user to start off with...
	d_ambient_light_contribution(0.4),
	d_globe_view_light_direction(1, 0, 0),
	d_map_view_light_direction(0, 0, 1)
{
	// By default lighting is only enabled for the following lighting primitives...
	d_lighting_primitives_enable_state.set(LIGHTING_SCALAR_FIELD);
	d_lighting_primitives_enable_state.set(LIGHTING_DIRECTION_ARROW);
}


void
GPlatesGui::SceneLightingParameters::set_ambient_light_contribution(
		const double &ambient_light_contribution)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			ambient_light_contribution >= 0 && ambient_light_contribution <= 1.0,
			GPLATES_ASSERTION_SOURCE);
	d_ambient_light_contribution = ambient_light_contribution;
}


bool
GPlatesGui::SceneLightingParameters::operator==(
		const SceneLightingParameters &rhs) const
{
	return
		d_lighting_primitives_enable_state == rhs.d_lighting_primitives_enable_state &&
		d_light_direction_attached_to_view_frame == rhs.d_light_direction_attached_to_view_frame &&
		GPlatesMaths::are_almost_exactly_equal(d_ambient_light_contribution, rhs.d_ambient_light_contribution) &&
		d_globe_view_light_direction == rhs.d_globe_view_light_direction &&
		d_map_view_light_direction == rhs.d_map_view_light_direction;
}


GPlatesMaths::UnitVector3D
GPlatesGui::transform_globe_view_space_light_direction_to_world_space(
		const GPlatesMaths::UnitVector3D &view_space_light_direction,
		const GPlatesMaths::Rotation &view_space_transform)
{
	// Need to reverse rotate from view-space back to world-space.
	return view_space_transform.get_reverse() * view_space_light_direction;
}


GPlatesMaths::UnitVector3D
GPlatesGui::transform_globe_view_space_light_direction_to_world_space(
		const GPlatesMaths::UnitVector3D &view_space_light_direction,
		const GPlatesOpenGL::GLMatrix &view_space_transform)
{
	// The light direction is in view-space.
	const double light_x = view_space_light_direction.x().dval();
	const double light_y = view_space_light_direction.y().dval();
	const double light_z = view_space_light_direction.z().dval();

	// Need to reverse rotate back to world-space.
	// We'll assume the view orientation matrix stores only a 3x3 rotation.
	// In which case the inverse matrix is the transpose.
	const GPlatesOpenGL::GLMatrix &view = view_space_transform;

	// Multiply the view-space light direction by the transpose of the 3x3 view transform.
	return GPlatesMaths::Vector3D(
			view.get_element(0,0) * light_x + view.get_element(1,0) * light_y + view.get_element(2,0) * light_z,
			view.get_element(0,1) * light_x + view.get_element(1,1) * light_y + view.get_element(2,1) * light_z,
			view.get_element(0,2) * light_x + view.get_element(1,2) * light_y + view.get_element(2,2) * light_z)
					.get_normalisation();
}


GPlatesMaths::UnitVector3D
GPlatesGui::transform_globe_world_space_light_direction_to_view_space(
		const GPlatesMaths::UnitVector3D &world_space_light_direction,
		const GPlatesMaths::Rotation &view_space_transform)
{
	// Rotate from world-space to view-space.
	return view_space_transform * world_space_light_direction;
}


GPlatesMaths::UnitVector3D
GPlatesGui::transform_globe_world_space_light_direction_to_view_space(
		const GPlatesMaths::UnitVector3D &world_space_light_direction,
		const GPlatesOpenGL::GLMatrix &view_space_transform)
{
	// The light direction is in world-space.
	const double light_x = world_space_light_direction.x().dval();
	const double light_y = world_space_light_direction.y().dval();
	const double light_z = world_space_light_direction.z().dval();

	// We'll assume the view orientation matrix stores only a 3x3 rotation.
	const GPlatesOpenGL::GLMatrix &view = view_space_transform;

	// Multiply the world-space light direction by the 3x3 view transform.
	return GPlatesMaths::Vector3D(
			view.get_element(0,0) * light_x + view.get_element(0,1) * light_y + view.get_element(0,2) * light_z,
			view.get_element(1,0) * light_x + view.get_element(1,1) * light_y + view.get_element(1,2) * light_z,
			view.get_element(2,0) * light_x + view.get_element(2,1) * light_y + view.get_element(2,2) * light_z)
					.get_normalisation();
}
