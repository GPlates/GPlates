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

#include "SetDeformationParametersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructLayerTask.h"
#include "app-logic/ReconstructParams.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::SetDeformationParametersDialog::SetDeformationParametersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state)
{
	setupUi(this);

	// Enable/disable strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setEnabled(
			show_strain_accumulation_checkbox->isChecked());

	setup_connections();
}


bool
GPlatesQtWidgets::SetDeformationParametersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		// NOTE: Make sure we get a 'const' pointer to the reconstruct layer task params
		// otherwise it will think we are modifying it which will mean the reconstruct
		// layer will think it needs to regenerate its reconstructed feature geometries.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		const GPlatesAppLogic::ReconstructLayerTask::Params *layer_task_params =
			dynamic_cast<const GPlatesAppLogic::ReconstructLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (!layer_task_params)
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

		// Handle delta t.
		spinbox_end_time->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_end_time());
		spinbox_begin_time->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_begin_time());
		spinbox_time_increment->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_time_increment());

		// Show deformed feature geometries.
		show_deformed_feature_geometries_checkbox->setChecked(
				visual_layer_params->get_show_deformed_feature_geometries());

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
GPlatesQtWidgets::SetDeformationParametersDialog::setup_connections()
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
			show_strain_accumulation_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_show_strain_accumulation_changed(int)));
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::handle_begin_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_end_time->setMaximum(value - spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::handle_end_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(value + spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::handle_time_increment_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(spinbox_end_time->value() + value);
	spinbox_end_time->setMaximum(spinbox_begin_time->value() - value);
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::react_show_strain_accumulation_changed(
		int state)
{
	// Enable/disable strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setEnabled(
			show_strain_accumulation_checkbox->isChecked());
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::handle_apply()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (!layer_task_params)
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
			GPlatesAppLogic::ReconstructParams reconstruct_params = layer_task_params->get_reconstruct_params();
			reconstruct_params.set_deformation_end_time(spinbox_end_time->value());
			reconstruct_params.set_deformation_begin_time(spinbox_begin_time->value());
			reconstruct_params.set_deformation_time_increment(spinbox_time_increment->value());
			layer_task_params->set_reconstruct_params(reconstruct_params);

			// If any reconstruct parameters were modified then 'ApplicationState::reconstruct()'
			// will get called here (at scope exit).
		}

		// Show deformed feature geometries.
		visual_layer_params->set_show_deformed_feature_geometries(
				show_deformed_feature_geometries_checkbox->isChecked());

		// Show strain accumulation.
		visual_layer_params->set_show_strain_accumulation(
				show_strain_accumulation_checkbox->isChecked());
		// Set strain accumulation scale.
		visual_layer_params->set_strain_accumulation_scale(
				strain_accumulation_scale_spinbox->value());
	}

	accept();
}
