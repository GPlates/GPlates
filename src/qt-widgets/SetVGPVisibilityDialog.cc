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

#include "SetVGPVisibilityDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructLayerParams.h"
#include "app-logic/ReconstructParams.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::SetVGPVisibilityDialog::SetVGPVisibilityDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state)
{
	setupUi(this);

	setup_connections();
}


bool
GPlatesQtWidgets::SetVGPVisibilityDialog::populate(
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
		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			return false;
		}

		// Handle earliest and latest times.
		const GPlatesPropertyValues::GeoTimeInstant &begin_time =
				layer_params->get_reconstruct_params().get_vgp_earliest_time();
		if (begin_time.is_distant_past())
		{
			spinbox_begin->setValue(0.0);
			checkbox_past->setChecked(true);
		}
		else
		{
			double begin_value = begin_time.is_distant_future() ? 0.0 : begin_time.value();
			spinbox_begin->setValue(begin_value);
			checkbox_past->setChecked(false);
		}
		const GPlatesPropertyValues::GeoTimeInstant &end_time =
				layer_params->get_reconstruct_params().get_vgp_latest_time();
		if (end_time.is_distant_future())
		{
			spinbox_end->setValue(0.0);
			checkbox_future->setChecked(true);
		}
		else
		{
			double end_value = end_time.is_distant_past() ? 0.0 : end_time.value();
			spinbox_end->setValue(end_value);
			checkbox_future->setChecked(false);
		}

		// Handle delta t.
		spinbox_delta->setValue(layer_params->get_reconstruct_params().get_vgp_delta_t());

		// Handle visibility setting.
		//
		// Note: We do this after setting the other GUI controls because this code relies on
		// their state to determine whether they should be enabled or not (this is currently
		// only the case for the begin/end times).
		GPlatesAppLogic::ReconstructParams::VGPVisibilitySetting visibility_setting =
			layer_params->get_reconstruct_params().get_vgp_visibility_setting();
		switch (visibility_setting)
		{
			case GPlatesAppLogic::ReconstructParams::ALWAYS_VISIBLE:
				radiobutton_always_visible->setChecked(true);
				handle_always_visible();
				break;

			case GPlatesAppLogic::ReconstructParams::TIME_WINDOW:
				radiobutton_time_window->setChecked(true);
				handle_time_window();
				break;

			case GPlatesAppLogic::ReconstructParams::DELTA_T_AROUND_AGE:
				radiobutton_delta_t_around_age->setChecked(true);
				handle_delta_t();
				break;
		}

		// Handle circular error.
		checkbox_error->setChecked(visual_layer_params->get_vgp_draw_circular_error());

		return true;
	}

	return false;
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::setup_connections()
{
	QObject::connect(
			radiobutton_always_visible,
			SIGNAL(clicked()),
			this,
			SLOT(handle_always_visible()));
	QObject::connect(
			radiobutton_time_window,
			SIGNAL(clicked()),
			this,
			SLOT(handle_time_window()));
	QObject::connect(
			radiobutton_delta_t_around_age,
			SIGNAL(clicked()),
			this,
			SLOT(handle_delta_t()));

	QObject::connect(
			checkbox_past,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_distant_past(bool)));
	QObject::connect(
			checkbox_future,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_distant_future(bool)));

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
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_apply()
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

			GPlatesAppLogic::ReconstructParams reconstruct_params = layer_params->get_reconstruct_params();

			// Handle visibility setting.
			if (radiobutton_always_visible->isChecked())
			{
				reconstruct_params.set_vgp_visibility_setting(GPlatesAppLogic::ReconstructParams::ALWAYS_VISIBLE);
			}
			else if (radiobutton_time_window->isChecked())
			{
				reconstruct_params.set_vgp_visibility_setting(GPlatesAppLogic::ReconstructParams::TIME_WINDOW);
			}
			else if (radiobutton_delta_t_around_age->isChecked())
			{
				reconstruct_params.set_vgp_visibility_setting(GPlatesAppLogic::ReconstructParams::DELTA_T_AROUND_AGE);
			}

			// Handle earliest and latest times.
			GPlatesPropertyValues::GeoTimeInstant begin_time = checkbox_past->isChecked() ?
					GPlatesPropertyValues::GeoTimeInstant::create_distant_past() :
					GPlatesPropertyValues::GeoTimeInstant(spinbox_begin->value());
			reconstruct_params.set_vgp_earliest_time(begin_time);
			GPlatesPropertyValues::GeoTimeInstant end_time = checkbox_future->isChecked() ?
					GPlatesPropertyValues::GeoTimeInstant::create_distant_future() :
					GPlatesPropertyValues::GeoTimeInstant(spinbox_end->value());
			reconstruct_params.set_vgp_latest_time(end_time);

			// Handle delta t.
			reconstruct_params.set_vgp_delta_t(spinbox_delta->value());

			layer_params->set_reconstruct_params(reconstruct_params);

			// If any reconstruct parameters were modified then 'ApplicationState::reconstruct()'
			// will get called here (at scope exit).
		}

		// Handle circular error.
		if (visual_layer_params->get_vgp_draw_circular_error() != checkbox_error->isChecked())
		{
			visual_layer_params->set_vgp_draw_circular_error(checkbox_error->isChecked());
		}
	}

	accept();
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_always_visible()
{
	spinbox_begin->setEnabled(false);
	label_begin->setEnabled(false);
	spinbox_end->setEnabled(false);
	label_end->setEnabled(false);

	spinbox_delta->setEnabled(false);
	label_delta_t->setEnabled(false);

	checkbox_past->setEnabled(false);
	checkbox_future->setEnabled(false);
	label_and->setEnabled(false);
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_time_window()
{
	spinbox_begin->setEnabled(!checkbox_past->isChecked());
	label_begin->setEnabled(!checkbox_past->isChecked());
	spinbox_end->setEnabled(!checkbox_future->isChecked());
	label_end->setEnabled(!checkbox_future->isChecked());

	spinbox_delta->setEnabled(false);
	label_delta_t->setEnabled(false);

	checkbox_past->setEnabled(true);
	checkbox_future->setEnabled(true);
	label_and->setEnabled(true);
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_delta_t()
{
	spinbox_begin->setEnabled(false);
	label_begin->setEnabled(false);
	spinbox_end->setEnabled(false);
	label_end->setEnabled(false);

	spinbox_delta->setEnabled(true);
	label_delta_t->setEnabled(true);

	checkbox_past->setEnabled(false);
	checkbox_future->setEnabled(false);
	label_and->setEnabled(false);
}
 

void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_distant_past(
		bool state)
{
	spinbox_begin->setEnabled(!state);
}


void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_distant_future(
		bool state)
{
	spinbox_end->setEnabled(!state);
}

