/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <boost/optional.hpp> 
#include <boost/shared_ptr.hpp>

#include "SetTopologyReconstructionParametersDialog.h"

#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructLayerParams.h"
#include "app-logic/ReconstructParams.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::SetTopologyReconstructionParametersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state)
{
	setupUi(this);

	// Show/hide line tessellation controls if enabling/disabling tessellation.
	detect_lifetime_widget->setVisible(
			enable_detect_lifetime_check_box->isChecked());

	// Show/hide line tessellation controls if enabling/disabling tessellation.
	line_tessellation_widget->setVisible(
			enable_line_tessellation_degrees_check_box->isChecked());

	// Show/hide strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setVisible(
			show_strain_accumulation_checkbox->isChecked());

	setup_connections();

	QtWidgetUtils::resize_based_on_size_hint(this);
}


bool
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		// NOTE: Make sure we get a 'const' pointer to the reconstruct layer params
		// otherwise it will think we are modifying it which will mean the reconstruct
		// layer will think it needs to regenerate its reconstructed feature geometries.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		const GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<const GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (!layer_params)
		{
			return false;
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		const GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<const GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			return false;
		}

		const GPlatesAppLogic::ReconstructParams &reconstruct_params = layer_params->get_reconstruct_params();

		// Handle delta t.
		spinbox_end_time->setValue(reconstruct_params.get_topology_reconstruction_end_time());
		spinbox_begin_time->setValue(reconstruct_params.get_topology_reconstruction_begin_time());
		spinbox_time_increment->setValue(reconstruct_params.get_topology_reconstruction_time_increment());

		// Deformed position interpolation.
		if (reconstruct_params.get_topology_deformation_use_natural_neighbour_interpolation())
		{
			natural_neighbour_radio_button->setChecked(true);
		}
		else
		{
			barycentric_radio_button->setChecked(true);
		}

		// Whether to start reconstruction at each feature's time of appearance, or use geometry import time.
		start_reconstruction_at_time_of_appearance_checkbox->setChecked(
				reconstruct_params.get_topology_reconstruction_use_time_of_appearance());

		// Line tessellation.
		enable_line_tessellation_degrees_check_box->setChecked(
				reconstruct_params.get_topology_reconstruction_enable_line_tessellation());
		line_tessellation_degrees_spinbox->setValue(
				reconstruct_params.get_topology_reconstruction_line_tessellation_degrees());
		line_tessellation_widget->setVisible(
				reconstruct_params.get_topology_reconstruction_enable_line_tessellation());

		// Lifetime detection.
		enable_detect_lifetime_check_box->setChecked(
				reconstruct_params.get_topology_reconstruction_enable_lifetime_detection());
		detect_lifetime_threshold_velocity_delta_spin_box->setValue(
				reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_velocity_delta());
		detect_lifetime_threshold_distance_to_boundary_spin_box->setValue(
				reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary());
		detect_lifetime_widget->setVisible(
				reconstruct_params.get_topology_reconstruction_enable_lifetime_detection());

		// Show topology-reconstructed feature geometries.
		show_reconstructed_feature_geometries_checkbox->setChecked(
				visual_layer_params->get_show_topology_reconstructed_feature_geometries());

		// Show strain accumulation.
		show_strain_accumulation_checkbox->setChecked(
				visual_layer_params->get_show_strain_accumulation());
		// Set strain accumulation scale.
		strain_accumulation_scale_spinbox->setValue(
				visual_layer_params->get_strain_accumulation_scale());

		return true;
	}

	return false;
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::setup_connections()
{
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_apply()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QObject::connect(
			spinbox_begin_time, SIGNAL(valueChanged(double)),
			this, SLOT(handle_begin_time_spinbox_changed(double)));
	QObject::connect(
			spinbox_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(handle_end_time_spinbox_changed(double)));
	QObject::connect(
			spinbox_time_increment, SIGNAL(valueChanged(double)),
			this, SLOT(handle_time_increment_spinbox_changed(double)));

	QObject::connect(
			enable_detect_lifetime_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_enable_detect_lifetime_changed(int)));

	QObject::connect(
			enable_line_tessellation_degrees_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_enable_line_tessellation_changed(int)));

	QObject::connect(
			show_strain_accumulation_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_show_strain_accumulation_changed(int)));
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_begin_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_end_time->setMaximum(value - spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_end_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(value + spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_time_increment_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(spinbox_end_time->value() + value);
	spinbox_end_time->setMaximum(spinbox_begin_time->value() - value);
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_enable_detect_lifetime_changed(
		int state)
{
	// Show/hide line tessellation controls if enabling/disabling tessellation.
	detect_lifetime_widget->setVisible(
			enable_detect_lifetime_check_box->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_enable_line_tessellation_changed(
		int state)
{
	// Show/hide line tessellation controls if enabling/disabling tessellation.
	line_tessellation_widget->setVisible(
			enable_line_tessellation_degrees_check_box->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_show_strain_accumulation_changed(
		int state)
{
	// Show/hide strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setVisible(
			show_strain_accumulation_checkbox->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_apply()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (!layer_params)
		{
			accept();
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			accept();
		}

		{
			// Delay any calls to 'ApplicationState::reconstruct()' until scope exit.
			GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(d_application_state);

			// Handle settings
			GPlatesAppLogic::ReconstructParams reconstruct_params = layer_params->get_reconstruct_params();

			reconstruct_params.set_topology_reconstruction_end_time(spinbox_end_time->value());
			reconstruct_params.set_topology_reconstruction_begin_time(spinbox_begin_time->value());
			reconstruct_params.set_topology_reconstruction_time_increment(spinbox_time_increment->value());

			// Whether to start reconstruction at each feature's time of appearance, or use geometry import time.
			reconstruct_params.set_topology_reconstruction_use_time_of_appearance(
					start_reconstruction_at_time_of_appearance_checkbox->isChecked());

			// Deformed position interpolation.
			reconstruct_params.set_topology_deformation_use_natural_neighbour_interpolation(
					natural_neighbour_radio_button->isChecked());

			// Line tessellation.
			reconstruct_params.set_topology_reconstruction_enable_line_tessellation(
					enable_line_tessellation_degrees_check_box->isChecked());
			reconstruct_params.set_topology_reconstruction_line_tessellation_degrees(
					line_tessellation_degrees_spinbox->value());

			// Lifetime detection.
			reconstruct_params.set_topology_reconstruction_enable_lifetime_detection(
					enable_detect_lifetime_check_box->isChecked());
			reconstruct_params.set_topology_reconstruction_lifetime_detection_threshold_velocity_delta(
					detect_lifetime_threshold_velocity_delta_spin_box->value());
			reconstruct_params.set_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary(
					detect_lifetime_threshold_distance_to_boundary_spin_box->value());

			layer_params->set_reconstruct_params(reconstruct_params);

			// If any reconstruct parameters were modified then 'ApplicationState::reconstruct()'
			// will get called here (at scope exit).
		}

		// Show topology-reconstructed feature geometries.
		visual_layer_params->set_show_topology_reconstructed_feature_geometries(
				show_reconstructed_feature_geometries_checkbox->isChecked());

		// Show strain accumulation.
		visual_layer_params->set_show_strain_accumulation(
				show_strain_accumulation_checkbox->isChecked());
		// Set strain accumulation scale.
		visual_layer_params->set_strain_accumulation_scale(
				strain_accumulation_scale_spinbox->value());
	}

	accept();
}
