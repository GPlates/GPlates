/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "SetVGPVisibilityDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/VGPRenderSettings.h"
#include "presentation/ViewState.h" 


GPlatesQtWidgets::SetVGPVisibilityDialog::SetVGPVisibilityDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_),
	d_view_state_ptr(&view_state_)
{
	setupUi(this);
	
	set_initial_state();
	
	setup_connections();

}	

void
GPlatesQtWidgets::SetVGPVisibilityDialog::set_initial_state()
{
// Initial state is DELTA_T_AROUND_AGE.
// FIXME: Grab these initial settings from the view state.
	radiobutton_delta_t_around_age->setChecked(true);
	spinbox_delta->setValue(GPlatesAppLogic::VGPRenderSettings::INITIAL_VGP_DELTA_T);
	spinbox_begin->setValue(0.);
	spinbox_end->setValue(0.);
	checkbox_past->setChecked(true);
	checkbox_future->setChecked(true);
	spinbox_begin->setEnabled(false);
	spinbox_end->setEnabled(false);
	
	handle_delta_t();	
}

void
GPlatesQtWidgets::SetVGPVisibilityDialog::setup_connections()
{
	QObject::connect(radiobutton_always_visible,SIGNAL(clicked()),this,SLOT(handle_always_visible()));
	QObject::connect(radiobutton_time_window,SIGNAL(clicked()),this,SLOT(handle_time_window()));
	QObject::connect(radiobutton_delta_t_around_age,SIGNAL(clicked()),this,SLOT(handle_delta_t()));
	QObject::connect(checkbox_past,SIGNAL(clicked(bool)),this,SLOT(handle_distant_past(bool)));
	QObject::connect(checkbox_future,SIGNAL(clicked(bool)),this,SLOT(handle_distant_future(bool)));
	QObject::connect(button_apply,SIGNAL(clicked(bool)),this,SLOT(handle_apply()));
	QObject::connect(button_cancel,SIGNAL(clicked(bool)),this,SLOT(reject()));		
}



void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_apply()
{
	GPlatesAppLogic::VGPRenderSettings &vgp_render_settings =
		d_view_state_ptr->get_vgp_render_settings();
		
	// Update view_state with the state of this dialog.
	if (radiobutton_always_visible->isChecked())
	{
		vgp_render_settings.set_vgp_visibility_setting(GPlatesAppLogic::VGPRenderSettings::ALWAYS_VISIBLE);
	}
	else if (radiobutton_time_window->isChecked())
	{
		vgp_render_settings.set_vgp_visibility_setting(GPlatesAppLogic::VGPRenderSettings::TIME_WINDOW);
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> begin_time;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> end_time;		
		if (checkbox_past->isChecked())
		{
			begin_time.reset(GPlatesPropertyValues::GeoTimeInstant::create_distant_past());
		}
		else
		{
			begin_time.reset(GPlatesPropertyValues::GeoTimeInstant(spinbox_begin->value()));			
		}
	
		if (checkbox_future->isChecked())
		{
			end_time.reset(GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
		}
		else
		{
			end_time.reset(GPlatesPropertyValues::GeoTimeInstant(spinbox_end->value()));			
		}		
		
		vgp_render_settings.set_vgp_earliest_time(*begin_time);
		vgp_render_settings.set_vgp_latest_time(*end_time);
		
	}
	else if (radiobutton_delta_t_around_age->isChecked())
	{
		vgp_render_settings.set_vgp_visibility_setting(GPlatesAppLogic::VGPRenderSettings::DELTA_T_AROUND_AGE);
		vgp_render_settings.set_vgp_delta_t(spinbox_delta->value());
	}
	accept();
	// Tell GPlates to reconstruct now so that the updated render settings are used. 
	d_view_state_ptr->get_application_state().reconstruct();	
}

void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_always_visible()
{
	spinbox_begin->setEnabled(false);
	spinbox_end->setEnabled(false);
	spinbox_delta->setEnabled(false);
	checkbox_past->setEnabled(false);
	checkbox_future->setEnabled(false);
}

void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_time_window()
{
	spinbox_begin->setEnabled(!checkbox_past->isChecked());
	spinbox_end->setEnabled(!checkbox_future->isChecked());
	spinbox_delta->setEnabled(false);
	checkbox_past->setEnabled(true);
	checkbox_future->setEnabled(true);
}

void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_delta_t()
{
	spinbox_begin->setEnabled(false);
	spinbox_end->setEnabled(false);
	spinbox_delta->setEnabled(true);
	checkbox_past->setEnabled(false);
	checkbox_future->setEnabled(false);
}
 
void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_distant_past(bool state)
{
	spinbox_begin->setEnabled(!state);
}

void
GPlatesQtWidgets::SetVGPVisibilityDialog::handle_distant_future(bool state)
{
	spinbox_end->setEnabled(!state);
}
