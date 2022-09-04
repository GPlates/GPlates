/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include "SceneOverlays.h"

#include "TextOverlay.h"
#include "VelocityLegendOverlay.h"

#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"


GPlatesGui::SceneOverlays::SceneOverlays(
		GPlatesPresentation::ViewState &view_state) :
	d_text_overlay_settings(view_state.get_text_overlay_settings()),
	d_text_overlay(new TextOverlay(view_state.get_application_state())),
	d_velocity_legend_overlay_settings(view_state.get_velocity_legend_overlay_settings()),
	d_velocity_legend_overlay(new VelocityLegendOverlay())
{
}


GPlatesGui::SceneOverlays::~SceneOverlays()
{
	// boost::scoped_ptr destructor needs complete type.
}


void
GPlatesGui::SceneOverlays::render(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int device_pixel_ratio)
{

	// Render the text overlay.
	//
	// We use the paint device dimensions (and not the canvas dimensions) in case the paint device
	// is not the canvas (eg, when rendering to a larger dimension SVG paint device).
	d_text_overlay->paint(
			gl,
			d_text_overlay_settings,
			// These are widget dimensions (not device pixels)...
			view_projection.get_viewport().width() / device_pixel_ratio,
			view_projection.get_viewport().height() / device_pixel_ratio,
			1.0f/*scale*/);

	// Render the velocity legend overlay.
	d_velocity_legend_overlay->paint(
			gl,
			d_velocity_legend_overlay_settings,
			// These are widget dimensions (not device pixels)...
			view_projection.get_viewport().width() / device_pixel_ratio,
			view_projection.get_viewport().height() / device_pixel_ratio,
			1.0f/*scale*/);
}
