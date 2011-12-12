/**
 * \file 
 * $Revision: 9477 $
 * $Date: 2010-08-24 05:36:12 +0200 (ti, 24 aug 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <QLocale>

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"
#include "maths/MathsUtils.h"
#include "model/types.h"
#include "SmallCircleWidget.h"
#include "CalculateStagePoleDialog.h"

namespace
{
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

GPlatesQtWidgets::CalculateStagePoleDialog::CalculateStagePoleDialog(
	SmallCircleWidget *small_circle_widget_ptr,
	GPlatesAppLogic::ApplicationState &application_state,
	QWidget *parent_):
		QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
		d_small_circle_widget_ptr(small_circle_widget_ptr),
		d_application_state(application_state),
		d_centre(GPlatesMaths::LatLonPoint(0.,0.))
{

	setupUi(this);

	QLocale locale_;
	lineedit_lat->setText(locale_.toString(0.,'f',2)+QString("°"));
	lineedit_lon->setText(locale_.toString(0.,'f',2)+QString("°"));


	QObject::connect(button_calculate,SIGNAL(clicked()),
		this,SLOT(handle_calculate()));
	QObject::connect(button_use,SIGNAL(clicked()),
		this,SLOT(handle_use()));
}

void
GPlatesQtWidgets::CalculateStagePoleDialog::handle_calculate()
{
	// Attempt to generate a stage pole from the plate id and time fields.
	// If it's possible, use the axis of the stage pole as the centre coordinates. 
	GPlatesModel::integer_plate_id_type p_moving = spinbox_plate_id_1->value();
	GPlatesModel::integer_plate_id_type p_fixed = spinbox_plate_id_2->value();
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
		d_application_state.get_current_reconstruction().get_default_reconstruction_layer_output()->get_reconstruction_tree();

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

	d_centre = get_axis_llp_from_rotation(stage_pole);

	QLocale locale_;
	lineedit_lat->setText(locale_.toString(d_centre.latitude(),'f',2)+QString("°"));
	lineedit_lon->setText(locale_.toString(d_centre.longitude(),'f',2)+QString("°"));
}

void
GPlatesQtWidgets::CalculateStagePoleDialog::handle_use()
{
	d_small_circle_widget_ptr->set_centre(d_centre);
}

