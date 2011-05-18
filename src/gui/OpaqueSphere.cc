/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2010 The University of Sydney, Australia
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

#include <cmath>
#include <vector>
#include <utility>

#include "OpaqueSphere.h"

#include "maths/MathsUtils.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeDrawable.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLTransform.h"
#include "opengl/GLUQuadric.h"
#include "opengl/GLVertex.h"

#include "presentation/ViewState.h"


namespace
{
	static const double RADIUS = 1.0;
	static const unsigned int NUM_SLICES = 72;

	typedef GPlatesOpenGL::GLColouredVertex vertex_type;
	typedef std::pair<double, double> double_pair;


	/**
	 * Computes the sin and cos of:
	 *     2 * PI * i / num_slices
	 * for 0 <= i <= num_slices.
	 *
	 * Note that the returned vector has (num_slices + 1) elements.
	 */
	std::vector<double_pair>
	compute_sin_cos_angles(
			unsigned int num_slices)
	{
		std::vector<double_pair> result;
		for (unsigned int i = 0; i <= num_slices; ++i)
		{
			double angle = 2 * GPlatesMaths::PI * i / num_slices;
			result.push_back(std::make_pair(std::sin(angle), std::cos(angle)));
		}
		return result;
	}


	/**
	 * Creates a donut-shaped drawable on the z = 0 plane.
	 */
	boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>
	create_disk_drawable(
			double inner_radius,
			double outer_radius,
			const std::vector<double_pair> &sin_cos_angles,
			const GPlatesGui::rgba8_t &inner_colour,
			const GPlatesGui::rgba8_t &outer_colour)
	{
		static const GLfloat Z_VALUE = 0;
		GPlatesOpenGL::GLStreamPrimitives<vertex_type>::non_null_ptr_type stream =
				GPlatesOpenGL::GLStreamPrimitives<vertex_type>::create();
		GPlatesOpenGL::GLStreamTriangleStrips<vertex_type> stream_triangle_strips(*stream);
		stream_triangle_strips.begin_triangle_strip();

		for (std::vector<double_pair>::const_iterator iter = sin_cos_angles.begin();
				iter != sin_cos_angles.end(); ++iter)
		{
			const vertex_type outer_vertex(
				static_cast<GLfloat>(outer_radius * iter->first),
				static_cast<GLfloat>(outer_radius * iter->second),
				Z_VALUE,
				outer_colour);
			const vertex_type inner_vertex(
				static_cast<GLfloat>(inner_radius * iter->first),
				static_cast<GLfloat>(inner_radius * iter->second),
				Z_VALUE,
				inner_colour);

			stream_triangle_strips.add_vertex(outer_vertex);
			stream_triangle_strips.add_vertex(inner_vertex);
		}

		stream_triangle_strips.end_triangle_strip();
		return stream->get_drawable();
	}


	/**
	 * Evaluates the integral of sqrt(r^2 - x^2) with respect to x for a given
	 * value of r and x (ignoring the constant of integration).
	 */
	double
	eval_integral(
			double x,
			double r)
	{
		double sqrt_part = std::sqrt(r * r - x * x);
		return 0.5 * (x * sqrt_part + r * r * std::atan2(x, sqrt_part));
	}


	/**
	 * Draws a disk on the z = 0 plane with varying translucency from centre to
	 * edge, that simulates what a real translucent sphere would look like.
	 *
	 * Imagine a translucent balloon, and consider parallel light rays travelling
	 * from behind the balloon towards the viewer. A light ray going through the
	 * centre of the balloon has to go through less material than a light ray going
	 * through the balloon further away from the centre, where the balloon's
	 * surface is more slanted relative to the viewer.
	 *
	 * We model this using a 2D doughnut (for ease of calculation), with an outer
	 * radius of RADIUS and a very small thickness. We calculate the amount
	 * of doughnut in each equal slice from x = 0 to x = RADIUS. The alpha
	 * value of @a colour is used at the centre of the disk, and as we go outwards,
	 * the alpha value is modulated by the amount of doughnut in that slice.
	 */
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	create_translucent_sphere_drawable(
			const GPlatesGui::rgba8_t &colour)
	{
		static const unsigned int STEPS = 150;
		static const double INNER_RADIUS = (STEPS - 0.5) / STEPS * RADIUS;

		double radii[STEPS + 1];
		double integral[STEPS + 1];
		double inner_integral[STEPS + 1];
		for (unsigned int i = 0; i <= STEPS; ++i)
		{
			double curr_radius = RADIUS * i / STEPS;
			radii[i] = curr_radius;
			integral[i] = eval_integral(curr_radius, RADIUS);
			inner_integral[i] = eval_integral(curr_radius > INNER_RADIUS ? INNER_RADIUS : curr_radius, INNER_RADIUS);
		}

		double thickness[STEPS];
		for (unsigned int i = 0; i < STEPS; ++i)
		{
			double outer_area = integral[i + 1] - integral[i];
			double inner_area = inner_integral[i + 1] - inner_integral[i];
			thickness[i] = outer_area - inner_area;
		}

		double base_thickness = thickness[0];
		double base_alpha = colour.alpha / 255.0;
		boost::uint8_t alphas[STEPS + 1];
		alphas[0] = colour.alpha;
		for (unsigned int i = 0; i < STEPS; ++i)
		{
			double alpha = 1.0 - std::pow(1.0 - base_alpha, thickness[i] / base_thickness);
			alphas[i + 1] = static_cast<boost::uint8_t>(alpha * 255.0);
		}

		GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
			GPlatesOpenGL::GLCompositeDrawable::create();
		std::vector<double_pair> sin_cos_angles = compute_sin_cos_angles(NUM_SLICES);

		for (unsigned int i = 0; i < STEPS; ++i)
		{
			GPlatesGui::rgba8_t inner_colour = colour;
			inner_colour.alpha = alphas[i];
			GPlatesGui::rgba8_t outer_colour = colour;
			outer_colour.alpha = alphas[i + 1];

			boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> disk_drawable =
				create_disk_drawable(radii[i], radii[i + 1], sin_cos_angles, inner_colour, outer_colour);
			if (disk_drawable)
			{
				composite_drawable->add_drawable(*disk_drawable);
			}
		}
		
		return composite_drawable;
	}


	/**
	 * Draws a disk on the z = 0 plane with a fixed @a colour.
	 */
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	create_opaque_sphere_drawable(
			const GPlatesGui::rgba8_t &colour)
	{
		GPlatesOpenGL::GLCompositeDrawable::non_null_ptr_type composite_drawable =
			GPlatesOpenGL::GLCompositeDrawable::create();
		std::vector<double_pair> sin_cos_angles = compute_sin_cos_angles(NUM_SLICES);
		boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> disk_drawable =
			create_disk_drawable(0.0, RADIUS, sin_cos_angles, colour, colour);
		if (disk_drawable)
		{
			composite_drawable->add_drawable(*disk_drawable);
		}

		return composite_drawable;
	}


	/**
	 * Creates a drawable that renders the sphere to the screen.
	 *
	 * Note that this actually draws a flat disk on the z = 0 plane instead of an
	 * actual sphere. This has the advantage that, if we draw to the depth buffer
	 * while drawing the disk, we can use a depth test to occlude geometries on the
	 * far side of the globe, but since the disk cuts through the centre of the
	 * globe, we avoid any artifacts due to geometries dipping in and out of the
	 * surface of the globe.
	 */
	GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
	create_sphere_drawable(
			const GPlatesGui::rgba8_t &colour)
	{
		if (colour.alpha == 255)
		{
			return create_opaque_sphere_drawable(colour);
		}
		else
		{
			return create_translucent_sphere_drawable(colour);
		}
	}


	GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
	create_sphere_state_set()
	{
		GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
		blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		return blend_state;
	}


	GPlatesOpenGL::GLTransform::non_null_ptr_type
	undo_rotation(
			const GPlatesMaths::UnitVector3D &axis,
			double angle_in_deg)
	{
		GPlatesOpenGL::GLTransform::non_null_ptr_type transform =
				GPlatesOpenGL::GLTransform::create(GL_MODELVIEW);
		// Undo the rotation done in Globe so that the disk always faces the camera.
		transform->get_matrix().gl_rotate(-angle_in_deg, axis.x().dval(), axis.y().dval(), axis.z().dval());
		// Rotate the axes so that the z-axis is perpendicular to the screen.
		// This is because we draw the disk on the z = 0 plane.
		transform->get_matrix().gl_rotate(90, 0.0, 1.0, 0.0);
		
		return transform;
	}
}


GPlatesGui::OpaqueSphere::OpaqueSphere(
		const Colour &colour) :
	d_view_state(NULL),
	d_colour(colour),
	d_drawable(create_sphere_drawable(Colour::to_rgba8(d_colour))),
	d_state_set(create_sphere_state_set())
{  }


GPlatesGui::OpaqueSphere::OpaqueSphere(
		const GPlatesPresentation::ViewState &view_state) :
	d_view_state(&view_state),
	d_colour(view_state.get_background_colour()),
	d_drawable(create_sphere_drawable(Colour::to_rgba8(d_colour))),
	d_state_set(create_sphere_state_set())
{  }


void
GPlatesGui::OpaqueSphere::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesMaths::UnitVector3D &axis,
		double angle_in_deg)
{
	// Check whether the view state's background colour has changed.
	if (d_view_state && d_view_state->get_background_colour() != d_colour)
	{
		d_colour = d_view_state->get_background_colour();
		d_drawable = create_sphere_drawable(Colour::to_rgba8(d_colour));
	}

	GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type transform = undo_rotation(axis, angle_in_deg);
	renderer.push_transform(*transform);
	renderer.push_state_set(d_state_set);
	renderer.add_drawable(d_drawable);
	renderer.pop_state_set();
	renderer.pop_transform();
}

