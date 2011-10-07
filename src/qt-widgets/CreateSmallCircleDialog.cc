/* $Id: PanMap.h 8194 2010-04-26 16:24:42Z rwatson $ */

/**
 * \file 
 * $Revision: 8194 $
 * $Date: 2010-04-26 18:24:42 +0200 (ma, 26 apr 2010) $ 
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

#include "CreateSmallCircleDialog.h"

#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"

#include "model/types.h"

namespace
{
	void
	set_widget_background_colour(
		QWidget *widget,
		const QColor &colour)
	{
		QPalette palette = widget->palette();
		palette.setColor(QPalette::Base, colour);
		widget->setPalette(palette);
	}

        /**
         * Check that the entered mulitple-radii fields make sense.
         *
         */
	bool
	fields_are_valid(
		double r1, 
		double r2,
		double dr)
	{
		if ((r1 <= 0.) ||
			(r2 < r1) ||
			(dr <= 0.))
		{
			return false;
		}
		return true;
	}

	GPlatesMaths::LatLonPoint
	get_axis_llp_from_rotation(
		const GPlatesMaths::FiniteRotation &rotation)
	{
		using namespace GPlatesMaths;
		
		if (represents_identity_rotation(rotation.unit_quat()))
		{
			return LatLonPoint(0,0);
		}

		const UnitQuaternion3D old_uq = rotation.unit_quat();

		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();		
		UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
		real_t angle = params.angle;

		PointOnSphere point(params.axis);
		return make_lat_lon_point(point);
	}
}

GPlatesQtWidgets::CreateSmallCircleDialog::CreateSmallCircleDialog(
	GPlatesQtWidgets::SmallCircleManager *small_circle_manager,
	GPlatesAppLogic::ApplicationState &application_state,
	QWidget *parent_):
		QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | 				Qt::WindowSystemMenuHint),
		d_small_circle_manager_ptr(small_circle_manager),
		d_application_state(application_state)
{
	setupUi(this);

	//initial states
	checkbox_stage_pole->setChecked(false);
        frame_stage_pole->setEnabled(false);
	spinbox_radius_1->setEnabled(false);
	spinbox_radius_2->setEnabled(false);
	spinbox_step->setEnabled(false);
	radio_button_single->setChecked(true);
	radio_button_multiple->setChecked(false);

	// connections
	QObject::connect(checkbox_stage_pole,SIGNAL(stateChanged(int)),this,SLOT(handle_stage_pole_checkbox_state()));
	QObject::connect(button_calculate_stage_pole,SIGNAL(clicked()),this,SLOT(handle_calculate()));
	QObject::connect(button_add,SIGNAL(clicked()),this,SLOT(handle_add()));
	QObject::connect(radio_button_single,SIGNAL(toggled(bool)),this,SLOT(handle_single_changed(bool)));
	QObject::connect(radio_button_multiple,SIGNAL(toggled(bool)),this,SLOT(handle_multiple_changed(bool)));
	QObject::connect(spinbox_radius_1,SIGNAL(valueChanged(double)),SLOT(handle_multiple_circle_fields_changed()));
	QObject::connect(spinbox_radius_2,SIGNAL(valueChanged(double)),SLOT(handle_multiple_circle_fields_changed()));
	QObject::connect(spinbox_step,SIGNAL(valueChanged(double)),SLOT(handle_multiple_circle_fields_changed()));

	QtWidgetUtils::resize_based_on_size_hint(this);
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_stage_pole_checkbox_state()
{
        frame_stage_pole->setEnabled(checkbox_stage_pole->isChecked());
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_calculate()
{
	// Attempt to generate a stage pole from the plate id and time fields.
	// If it's possible, use the axis of the stage pole as the centre coordinates. 
	GPlatesModel::integer_plate_id_type p_moving = spinbox_plate_id_moving->value();
	GPlatesModel::integer_plate_id_type p_fixed = spinbox_plate_id_fixed->value();
	double t1 = spinbox_time_1->value();
	double t2 = spinbox_time_2->value();

	if (GPlatesMaths::are_almost_exactly_equal(t1,t2) ||
		(p_moving == p_fixed))
	{
		return;
	}

	// To create new trees, we need to know which reconstruction features should be used.
	// We'll use the same features that have been used for the default reconstruction tree. 
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type default_tree = 	
		d_application_state.get_current_reconstruction()
			.get_default_reconstruction_layer_output()->get_reconstruction_tree();

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_1 = 
		GPlatesAppLogic::create_reconstruction_tree(
		t1,
		d_application_state.get_current_anchored_plate_id(),
		default_tree->get_reconstruction_features());

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_2 = 
		GPlatesAppLogic::create_reconstruction_tree(
		t2,
		d_application_state.get_current_anchored_plate_id(),
		default_tree->get_reconstruction_features());

	// Get stage pole
	GPlatesMaths::FiniteRotation stage_pole =
		GPlatesAppLogic::ReconstructUtils::get_stage_pole(
			*tree_1,
			*tree_2,				
			p_moving,
			p_fixed);

	GPlatesMaths::LatLonPoint llp = get_axis_llp_from_rotation(stage_pole);

	spinbox_lat->setValue(llp.latitude());
	spinbox_lon->setValue(llp.longitude());

}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_add()
{
	using namespace GPlatesMaths;

	PointOnSphere centre = make_point_on_sphere(LatLonPoint(spinbox_lat->value(),spinbox_lon->value()));

	bool valid_circle_added = false;

	if (radio_button_single->isChecked())
	{
		Real radius = spin_box_radius->value();

		// Add a single circle to the collection.

		// Prevent zero radius. This should not be possible anyway, due to UI designer settings.
		if (radius == 0.)
		{
			return;
		}

		const Real colat = convert_deg_to_rad(radius);

		d_small_circle_manager_ptr->add_circle(
				GPlatesMaths::SmallCircle::create_colatitude(centre.position_vector(), colat));
		valid_circle_added = true;
	}

	if (radio_button_multiple->isChecked())
	{
		double r1 = spinbox_radius_1->value();
		double r2 = spinbox_radius_2->value();
		double dr = spinbox_step->value();

		if (fields_are_valid(r1,r2,dr))
		{


			for (double r = r1 ; r <= r2 ; r+= dr)
			{
				Real radius(r);

				// The earlier check on field validity should prevent zero radius anyway.
				if (radius == 0.)
				{
					continue;
				}

				const Real colat = convert_deg_to_rad(radius);

				d_small_circle_manager_ptr->add_circle(
						GPlatesMaths::SmallCircle::create_colatitude(centre.position_vector(), colat));
				valid_circle_added = true;
			}
		}
		else
		{
			highlight_invalid_radius_fields();
		}

	}
	
	if (valid_circle_added)
	{
		emit circle_added();

		// FIXME: We're close the dialog after each new small circle has been created. This
		// might get annoying for someone who has a whole bunch of circles to add.  Consider
		// leaving dialog open and updating the manager dialog, and the globe/map, after 
		// adding a small circle. 
		reject();
	}

}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_single_changed(
	bool state)
{
	spin_box_radius->setEnabled(state);
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_multiple_changed(
	bool state)
{
	spinbox_radius_1->setEnabled(state);
	spinbox_radius_2->setEnabled(state);
	spinbox_step->setEnabled(state);
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::init()
{
	// Set the first of the time spinboxes to the current reconstruction time. 
	spinbox_time_1->setValue(d_application_state.get_current_reconstruction_time());
}


void
GPlatesQtWidgets::CreateSmallCircleDialog::highlight_invalid_radius_fields()
{
	set_multiple_circle_field_colours(Qt::red);
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::handle_multiple_circle_fields_changed()
{
	set_multiple_circle_field_colours(Qt::white);
}

void
GPlatesQtWidgets::CreateSmallCircleDialog::set_multiple_circle_field_colours(
	const QColor &colour)
{
	set_widget_background_colour(spinbox_radius_1,colour);
	set_widget_background_colour(spinbox_radius_2,colour);
	set_widget_background_colour(spinbox_step,colour);
}
