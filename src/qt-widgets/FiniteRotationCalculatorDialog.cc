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

#include <cmath>
#include <QDebug>

#include "FiniteRotationCalculatorDialog.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/types.h"


GPlatesQtWidgets::FiniteRotationCalculatorDialog::FiniteRotationCalculatorDialog(
		QWidget *parent_):
	GPlatesDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	install_event_filters();

	make_signal_slot_connections();

	// Set the initial default dialog button.
	add_finite_rotations_button->setDefault(true);
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::make_signal_slot_connections()
{
	// Closing the dialog.
	QObject::connect(
		main_buttonbox,
		SIGNAL(rejected()),
		this,
		SLOT(reject()));

	QGroupBox::connect(
		add_finite_rotations_button,
		SIGNAL(clicked()),
		this,
		SLOT(handle_add_finite_rotations()));	

	QGroupBox::connect(
		compute_difference_rotation_button,
		SIGNAL(clicked()),
		this,
		SLOT(handle_compute_difference_rotation()));	

	QGroupBox::connect(
		calc_rotation_between_points_button,
		SIGNAL(clicked()),
		this,
		SLOT(handle_calc_rotation_between_points()));

	QGroupBox::connect(
		rotate_a_point_button,
		SIGNAL(clicked()),
		this,
		SLOT(handle_rotate_a_point()));	


	// Handle input changes to the 'add finite rotations' calculator.
	QGroupBox::connect(
		add_finite_rotations_rotation1_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	
	QGroupBox::connect(
		add_finite_rotations_rotation1_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	
	QGroupBox::connect(
		add_finite_rotations_rotation1_angle_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	
	QGroupBox::connect(
		add_finite_rotations_rotation2_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	
	QGroupBox::connect(
		add_finite_rotations_rotation2_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	
	QGroupBox::connect(
		add_finite_rotations_rotation2_angle_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_add_finite_rotations_input_changed()));	

	// Handle input changes to the 'subtract finite rotations' calculator.
	QGroupBox::connect(
		compute_difference_rotation_rotation1_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	
	QGroupBox::connect(
		compute_difference_rotation_rotation1_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	
	QGroupBox::connect(
		compute_difference_rotation_rotation1_angle_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	
	QGroupBox::connect(
		compute_difference_rotation_rotation2_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	
	QGroupBox::connect(
		compute_difference_rotation_rotation2_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	
	QGroupBox::connect(
		compute_difference_rotation_rotation2_angle_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_compute_difference_rotation_input_changed()));	

	// Handle input changes to the 'finite rotation between points' calculator.
	QGroupBox::connect(
		calc_rotation_between_points_initial_point_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_calc_rotation_between_points_input_changed()));	
	QGroupBox::connect(
		calc_rotation_between_points_initial_point_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_calc_rotation_between_points_input_changed()));	
	QGroupBox::connect(
		calc_rotation_between_points_final_point_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_calc_rotation_between_points_input_changed()));	
	QGroupBox::connect(
		calc_rotation_between_points_final_point_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_calc_rotation_between_points_input_changed()));	

	// Handle input changes to the 'rotate a point' calculator.
	QGroupBox::connect(
		rotate_a_point_initial_point_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_rotate_a_point_input_changed()));	
	QGroupBox::connect(
		rotate_a_point_initial_point_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_rotate_a_point_input_changed()));	
	QGroupBox::connect(
		rotate_a_point_rotation_lat_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_rotate_a_point_input_changed()));	
	QGroupBox::connect(
		rotate_a_point_rotation_lon_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_rotate_a_point_input_changed()));	
	QGroupBox::connect(
		rotate_a_point_rotation_angle_spinbox, SIGNAL(valueChanged(double)),
		this, SLOT(handle_rotate_a_point_input_changed()));	
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::install_event_filters()
{
	//
	// An event filter to change the default dialog button when the focus changes between inputs.
	//

	add_finite_rotations_rotation1_lat_spinbox->installEventFilter(this);
	add_finite_rotations_rotation1_lon_spinbox->installEventFilter(this);
	add_finite_rotations_rotation1_angle_spinbox->installEventFilter(this);
	add_finite_rotations_rotation2_lat_spinbox->installEventFilter(this);
	add_finite_rotations_rotation2_lon_spinbox->installEventFilter(this);
	add_finite_rotations_rotation2_angle_spinbox->installEventFilter(this);

	compute_difference_rotation_rotation1_lat_spinbox->installEventFilter(this);
	compute_difference_rotation_rotation1_lon_spinbox->installEventFilter(this);
	compute_difference_rotation_rotation1_angle_spinbox->installEventFilter(this);
	compute_difference_rotation_rotation2_lat_spinbox->installEventFilter(this);
	compute_difference_rotation_rotation2_lon_spinbox->installEventFilter(this);
	compute_difference_rotation_rotation2_angle_spinbox->installEventFilter(this);

	calc_rotation_between_points_initial_point_lat_spinbox->installEventFilter(this);
	calc_rotation_between_points_initial_point_lon_spinbox->installEventFilter(this);
	calc_rotation_between_points_final_point_lat_spinbox->installEventFilter(this);
	calc_rotation_between_points_final_point_lon_spinbox->installEventFilter(this);

	rotate_a_point_initial_point_lat_spinbox->installEventFilter(this);
	rotate_a_point_initial_point_lon_spinbox->installEventFilter(this);
	rotate_a_point_rotation_lat_spinbox->installEventFilter(this);
	rotate_a_point_rotation_lon_spinbox->installEventFilter(this);
	rotate_a_point_rotation_angle_spinbox->installEventFilter(this);
}


bool
GPlatesQtWidgets::FiniteRotationCalculatorDialog::eventFilter(
		QObject *watched,
		QEvent *ev)
{
	// If any calculator inputs have received focus then set the default dialog button
	// (the button that gets selected when user presses Enter) to the appropriate calculate button.
	if (ev->type() == QEvent::FocusIn)
	{
		if (watched == add_finite_rotations_rotation1_lat_spinbox ||
			watched == add_finite_rotations_rotation1_lon_spinbox ||
			watched == add_finite_rotations_rotation1_angle_spinbox ||
			watched == add_finite_rotations_rotation2_lat_spinbox ||
			watched == add_finite_rotations_rotation2_lon_spinbox ||
			watched == add_finite_rotations_rotation2_angle_spinbox)
		{
			add_finite_rotations_button->setDefault(true);
		}

		if (watched == compute_difference_rotation_rotation1_lat_spinbox ||
			watched == compute_difference_rotation_rotation1_lon_spinbox ||
			watched == compute_difference_rotation_rotation1_angle_spinbox ||
			watched == compute_difference_rotation_rotation2_lat_spinbox ||
			watched == compute_difference_rotation_rotation2_lon_spinbox ||
			watched == compute_difference_rotation_rotation2_angle_spinbox)
		{
			compute_difference_rotation_button->setDefault(true);
		}

		if (watched == calc_rotation_between_points_initial_point_lat_spinbox ||
			watched == calc_rotation_between_points_initial_point_lon_spinbox ||
			watched == calc_rotation_between_points_final_point_lat_spinbox ||
			watched == calc_rotation_between_points_final_point_lon_spinbox)
		{
			calc_rotation_between_points_button->setDefault(true);
		}

		if (watched == rotate_a_point_initial_point_lat_spinbox ||
			watched == rotate_a_point_initial_point_lon_spinbox ||
			watched == rotate_a_point_rotation_lat_spinbox ||
			watched == rotate_a_point_rotation_lon_spinbox ||
			watched == rotate_a_point_rotation_angle_spinbox)
		{
			rotate_a_point_button->setDefault(true);
		}
	}

	return false;
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_add_finite_rotations()
{
	const double rotation1_lat = add_finite_rotations_rotation1_lat_spinbox->value();
	const double rotation1_lon = add_finite_rotations_rotation1_lon_spinbox->value();
	const double rotation1_angle =
			GPlatesMaths::convert_deg_to_rad(add_finite_rotations_rotation1_angle_spinbox->value());

	const double rotation2_lat = add_finite_rotations_rotation2_lat_spinbox->value();
	const double rotation2_lon = add_finite_rotations_rotation2_lon_spinbox->value();
	const double rotation2_angle =
			GPlatesMaths::convert_deg_to_rad(add_finite_rotations_rotation2_angle_spinbox->value());

	const GPlatesMaths::UnitVector3D rotation1_axis = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(rotation1_lat, rotation1_lon)).position_vector();

	const GPlatesMaths::UnitVector3D rotation2_axis = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(rotation2_lat, rotation2_lon)).position_vector();

	const GPlatesMaths::Rotation rotation1 =
			GPlatesMaths::Rotation::create(rotation1_axis, rotation1_angle);

	const GPlatesMaths::Rotation rotation2 =
			GPlatesMaths::Rotation::create(rotation2_axis, rotation2_angle);

	// Apply 'rotation1' first since the GUI states that we are calculating 'rotation1 + rotation2'
	// which means a point is first rotated by 'rotation1' and then 'rotation2'.
	const GPlatesMaths::Rotation final_rotation = rotation2 * rotation1;

	const GPlatesMaths::LatLonPoint final_rotation_axis =
			make_lat_lon_point(GPlatesMaths::PointOnSphere(final_rotation.axis()));

	const double final_rotation_angle =
			GPlatesMaths::convert_rad_to_deg(final_rotation.angle().dval());

	add_finite_rotations_final_rotation_lat_lineedit->setText(
			QString::number(final_rotation_axis.latitude(), 'f', 4));
	add_finite_rotations_final_rotation_lon_lineedit->setText(
			QString::number(final_rotation_axis.longitude(), 'f', 4));
	add_finite_rotations_final_rotation_angle_lineedit->setText(
			QString::number(final_rotation_angle, 'f', 4));
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_compute_difference_rotation()
{
	const double rotation1_lat = compute_difference_rotation_rotation1_lat_spinbox->value();
	const double rotation1_lon = compute_difference_rotation_rotation1_lon_spinbox->value();
	const double rotation1_angle =
			GPlatesMaths::convert_deg_to_rad(compute_difference_rotation_rotation1_angle_spinbox->value());

	const double rotation2_lat = compute_difference_rotation_rotation2_lat_spinbox->value();
	const double rotation2_lon = compute_difference_rotation_rotation2_lon_spinbox->value();
	const double rotation2_angle =
			GPlatesMaths::convert_deg_to_rad(compute_difference_rotation_rotation2_angle_spinbox->value());

	const GPlatesMaths::UnitVector3D rotation1_axis = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(rotation1_lat, rotation1_lon)).position_vector();

	const GPlatesMaths::UnitVector3D rotation2_axis = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(rotation2_lat, rotation2_lon)).position_vector();

	const GPlatesMaths::Rotation rotation1 =
			GPlatesMaths::Rotation::create(rotation1_axis, rotation1_angle);

	const GPlatesMaths::Rotation rotation2 =
			GPlatesMaths::Rotation::create(rotation2_axis, rotation2_angle);

	// Apply '-rotation1' first since the GUI states that we are calculating 'reverse(rotation1) + rotation2'
	// which means a point is first rotated by '-rotation1' and then 'rotation2'.
	const GPlatesMaths::Rotation final_rotation = rotation2 * rotation1.get_reverse();

	const GPlatesMaths::LatLonPoint final_rotation_axis =
			make_lat_lon_point(GPlatesMaths::PointOnSphere(final_rotation.axis()));

	const double final_rotation_angle =
			GPlatesMaths::convert_rad_to_deg(final_rotation.angle().dval());

	compute_difference_rotation_final_rotation_lat_lineedit->setText(
			QString::number(final_rotation_axis.latitude(), 'f', 4));
	compute_difference_rotation_final_rotation_lon_lineedit->setText(
			QString::number(final_rotation_axis.longitude(), 'f', 4));
	compute_difference_rotation_final_rotation_angle_lineedit->setText(
			QString::number(final_rotation_angle, 'f', 4));
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_calc_rotation_between_points()
{
	const double initial_point_lat = calc_rotation_between_points_initial_point_lat_spinbox->value();
	const double initial_point_lon = calc_rotation_between_points_initial_point_lon_spinbox->value();

	const double final_point_lat = calc_rotation_between_points_final_point_lat_spinbox->value();
	const double final_point_lon = calc_rotation_between_points_final_point_lon_spinbox->value();

	const GPlatesMaths::PointOnSphere initial_point = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(initial_point_lat, initial_point_lon));

	const GPlatesMaths::PointOnSphere final_point = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(final_point_lat, final_point_lon));

	const GPlatesMaths::Rotation final_rotation =
			GPlatesMaths::Rotation::create(initial_point, final_point);

	const GPlatesMaths::LatLonPoint final_rotation_axis =
			make_lat_lon_point(GPlatesMaths::PointOnSphere(final_rotation.axis()));

	const double final_rotation_angle =
			GPlatesMaths::convert_rad_to_deg(final_rotation.angle().dval());

	calc_rotation_between_points_final_rotation_lat_lineedit->setText(
			QString::number(final_rotation_axis.latitude(), 'f', 4));
	calc_rotation_between_points_final_rotation_lon_lineedit->setText(
			QString::number(final_rotation_axis.longitude(), 'f', 4));
	calc_rotation_between_points_final_rotation_angle_lineedit->setText(
			QString::number(final_rotation_angle, 'f', 4));
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_rotate_a_point()
{
	const double initial_point_lat = rotate_a_point_initial_point_lat_spinbox->value();
	const double initial_point_lon = rotate_a_point_initial_point_lon_spinbox->value();

	const double rotation_lat = rotate_a_point_rotation_lat_spinbox->value();
	const double rotation_lon = rotate_a_point_rotation_lon_spinbox->value();
	const double rotation_angle =
			GPlatesMaths::convert_deg_to_rad(rotate_a_point_rotation_angle_spinbox->value());

	const GPlatesMaths::PointOnSphere initial_point = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(initial_point_lat, initial_point_lon));

	const GPlatesMaths::UnitVector3D rotation_axis = make_point_on_sphere(
			GPlatesMaths::LatLonPoint(rotation_lat, rotation_lon)).position_vector();

	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis, rotation_angle);

	const GPlatesMaths::LatLonPoint final_point = make_lat_lon_point(rotation * initial_point);

	rotate_a_point_final_point_lat_lineedit->setText(
			QString::number(final_point.latitude(), 'f', 4));
	rotate_a_point_final_point_lon_lineedit->setText(
			QString::number(final_point.longitude(), 'f', 4));
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_add_finite_rotations_input_changed()
{
	add_finite_rotations_final_rotation_lat_lineedit->clear();
	add_finite_rotations_final_rotation_lon_lineedit->clear();
	add_finite_rotations_final_rotation_angle_lineedit->clear();
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_compute_difference_rotation_input_changed()
{
	compute_difference_rotation_final_rotation_lat_lineedit->clear();
	compute_difference_rotation_final_rotation_lon_lineedit->clear();
	compute_difference_rotation_final_rotation_angle_lineedit->clear();
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_calc_rotation_between_points_input_changed()
{
	calc_rotation_between_points_final_rotation_lat_lineedit->clear();
	calc_rotation_between_points_final_rotation_lon_lineedit->clear();
	calc_rotation_between_points_final_rotation_angle_lineedit->clear();
}


void
GPlatesQtWidgets::FiniteRotationCalculatorDialog::handle_rotate_a_point_input_changed()
{
	rotate_a_point_final_point_lat_lineedit->clear();
	rotate_a_point_final_point_lon_lineedit->clear();
}
