/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <QtGlobal>
#include <QDebug>

#include "LightingWidget.h"

#include "GlobeAndMapWidget.h"
#include "GlobeCanvas.h"
#include "ReconstructionViewWidget.h"
#include "ViewportWindow.h"

#include "gui/SceneLightingParameters.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::LightingWidget::LightingWidget(
		ViewportWindow &viewport_window_,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_view_state(viewport_window_.get_view_state()),
	d_globe_and_map_widget(viewport_window_.reconstruction_view_widget().globe_and_map_widget())
{
	setupUi(this);

	// Initialise the widget state based on the scene lighting parameters.

	const GPlatesGui::SceneLightingParameters &scene_lighting_parameters =
			d_view_state.get_scene_lighting_parameters();
	enable_lighting_geometry_on_sphere->setChecked(
			scene_lighting_parameters.is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_GEOMETRY_ON_SPHERE));
	enable_lighting_arrow->setChecked(
			scene_lighting_parameters.is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW));
	enable_lighting_filled_geometry_on_sphere->setChecked(
			scene_lighting_parameters.is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_FILLED_GEOMETRY_ON_SPHERE));
	enable_lighting_raster->setChecked(
			scene_lighting_parameters.is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_RASTER));
	enable_lighting_scalar_field->setChecked(
			scene_lighting_parameters.is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_SCALAR_FIELD));
	ambient_lighting_spin_box->setValue(
			scene_lighting_parameters.get_ambient_light_contribution());
	light_direction_attached_to_view_frame_check_box->setChecked(
			scene_lighting_parameters.is_light_direction_attached_to_view_frame());

	make_signal_slot_connections();
}


GPlatesQtWidgets::LightingWidget::~LightingWidget()
{
	// boost::scoped_ptr destructor requires complete type.
}


void
GPlatesQtWidgets::LightingWidget::handle_activation()
{
}


void
GPlatesQtWidgets::LightingWidget::react_enable_lighting_geometry_on_sphere_check_box_changed()
{
	d_view_state.get_scene_lighting_parameters().enable_lighting(
			GPlatesGui::SceneLightingParameters::LIGHTING_GEOMETRY_ON_SPHERE,
			enable_lighting_geometry_on_sphere->isChecked());

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_enable_lighting_filled_geometry_on_sphere_check_box_changed()
{
	d_view_state.get_scene_lighting_parameters().enable_lighting(
			GPlatesGui::SceneLightingParameters::LIGHTING_FILLED_GEOMETRY_ON_SPHERE,
			enable_lighting_filled_geometry_on_sphere->isChecked());

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_enable_lighting_arrow_check_box_changed()
{
	d_view_state.get_scene_lighting_parameters().enable_lighting(
			GPlatesGui::SceneLightingParameters::LIGHTING_DIRECTION_ARROW,
			enable_lighting_arrow->isChecked());

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_enable_lighting_raster_check_box_changed()
{
	d_view_state.get_scene_lighting_parameters().enable_lighting(
			GPlatesGui::SceneLightingParameters::LIGHTING_RASTER,
			enable_lighting_raster->isChecked());

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_enable_lighting_scalar_field_check_box_changed()
{
	d_view_state.get_scene_lighting_parameters().enable_lighting(
			GPlatesGui::SceneLightingParameters::LIGHTING_SCALAR_FIELD,
			enable_lighting_scalar_field->isChecked());

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_ambient_lighting_spinbox_changed(
		double value)
{
	d_view_state.get_scene_lighting_parameters().set_ambient_light_contribution(value);

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::react_light_direction_attached_to_view_frame_check_box_changed()
{
	const bool light_direction_attached_to_view_frame =
			light_direction_attached_to_view_frame_check_box->isChecked();

	const GPlatesMaths::Rotation &view_space_transform =
			d_globe_and_map_widget.get_globe_canvas().globe().orientation().rotation();

	GPlatesGui::SceneLightingParameters &scene_lighting_parameters =
			d_view_state.get_scene_lighting_parameters();

	if (light_direction_attached_to_view_frame)
	{
		// Light direction was previously in world-space so transform it to view-space so that it
		// doesn't jump directions (the GLLight class transforms it back to world-space before lighting).
		scene_lighting_parameters.set_globe_view_light_direction(
				GPlatesGui::transform_globe_world_space_light_direction_to_view_space(
						scene_lighting_parameters.get_globe_view_light_direction(),
						view_space_transform));
	}
	else
	{
		// Light direction was previously in view-space so transform it to world-space so that it
		// doesn't jump directions (the GLLight class will use the world-space before lighting).
		scene_lighting_parameters.set_globe_view_light_direction(
				GPlatesGui::transform_globe_view_space_light_direction_to_world_space(
						scene_lighting_parameters.get_globe_view_light_direction(),
						view_space_transform));
	}

	scene_lighting_parameters.set_light_direction_attached_to_view_frame(
			light_direction_attached_to_view_frame);

	apply_lighting();
}


void
GPlatesQtWidgets::LightingWidget::apply_lighting()
{
	// Force the globe or map canvas to redraw itself with the updated lighting.
	d_globe_and_map_widget.update_canvas();
}


void
GPlatesQtWidgets::LightingWidget::make_signal_slot_connections()
{
	QObject::connect(
			enable_lighting_geometry_on_sphere, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_lighting_geometry_on_sphere_check_box_changed()));
	QObject::connect(
			enable_lighting_arrow, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_lighting_arrow_check_box_changed()));
	QObject::connect(
			enable_lighting_filled_geometry_on_sphere, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_lighting_filled_geometry_on_sphere_check_box_changed()));
	QObject::connect(
			enable_lighting_raster, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_lighting_raster_check_box_changed()));
	QObject::connect(
			enable_lighting_scalar_field, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_lighting_scalar_field_check_box_changed()));
	QObject::connect(
			ambient_lighting_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(react_ambient_lighting_spinbox_changed(double)));
	QObject::connect(
			light_direction_attached_to_view_frame_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(react_light_direction_attached_to_view_frame_check_box_changed()));
}
