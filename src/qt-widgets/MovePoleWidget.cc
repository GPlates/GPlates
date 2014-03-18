/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "MovePoleWidget.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"


GPlatesQtWidgets::MovePoleWidget::MovePoleWidget(
		QWidget *parent_):
	TaskPanelWidget(parent_)
{
	setupUi(this);

	// Initialise the widget state based on the pole.

	enable_pole_checkbox->setChecked(d_pole);
	pole_groupbox->setEnabled(d_pole);

	if (d_pole)
	{
		const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

		latitude_spinbox->setValue(lat_lon_pole.latitude());
		longitude_spinbox->setValue(lat_lon_pole.longitude());
	}
	else // set to the north pole...
	{
		latitude_spinbox->setValue(90);
		longitude_spinbox->setValue(0);
	}

	make_signal_slot_connections();
}


GPlatesQtWidgets::MovePoleWidget::~MovePoleWidget()
{
	// boost::scoped_ptr destructor requires complete type.
}


void
GPlatesQtWidgets::MovePoleWidget::set_pole(
		boost::optional<GPlatesMaths::PointOnSphere> pole)
{
	// Return early if no state change.
	if (pole == d_pole)
	{
		return;
	}

	d_pole = pole;

	// Disconnect enable pole checkbox and lat/lon spinbox slots.
	// We only want to emit 'pole_changed' signal once.
	QObject::disconnect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::disconnect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::disconnect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	// Enable or disable pole.
	enable_pole_checkbox->setChecked(d_pole);

	if (d_pole)
	{
		const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

		latitude_spinbox->setValue(lat_lon_pole.latitude());
		longitude_spinbox->setValue(lat_lon_pole.longitude());
	}

	// Reconnect enable pole checkbox and lat/lon spinbox slots.
	QObject::connect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::connect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::connect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::activate()
{
	// Enable the task panel widget.
	setEnabled(true);
}


void
GPlatesQtWidgets::MovePoleWidget::deactivate()
{
	// Disable the task panel widget.
	setEnabled(false);
}


void
GPlatesQtWidgets::MovePoleWidget::react_enable_pole_check_box_changed()
{
	// Return early if no state change.
	if (enable_pole_checkbox->isChecked() == bool(d_pole))
	{
		return;
	}

	if (enable_pole_checkbox->isChecked())
	{
		d_pole = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(latitude_spinbox->value(), longitude_spinbox->value()));
	}
	else
	{
		d_pole = boost::none;
	}

	// Enable/disable ability to modify the pole.
	pole_groupbox->setEnabled(d_pole);

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_latitude_spinbox_changed()
{
	// Should only be able to change spinbox value if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(latitude_spinbox->value(), lat_lon_pole.latitude()))
	{
		return;
	}

	// NOTE: Use longitude spinbox value instead of 'd_pole' longitude value because the later gets
	// reset by PointOnSphere to LatLonPoint conversion at the north/south pole.
	d_pole = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(latitude_spinbox->value(), longitude_spinbox->value()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_longitude_spinbox_changed()
{
	// Should only be able to change spinbox value if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(longitude_spinbox->value(), lat_lon_pole.longitude()))
	{
		return;
	}

	d_pole = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(lat_lon_pole.latitude(), longitude_spinbox->value()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_north_pole_pushbutton_clicked()
{
	// Should only be able to set north pole if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(90, lat_lon_pole.latitude()) &&
		GPlatesMaths::are_almost_exactly_equal(0, lat_lon_pole.longitude()))
	{
		return;
	}

	d_pole = GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90, 0));

	// Disconnect lat/lon spinbox slots - we only want to emit 'pole_changed' signal once.
	QObject::disconnect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::disconnect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	latitude_spinbox->setValue(90);
	longitude_spinbox->setValue(0);

	// Reconnect lat/lon spinbox slots.
	QObject::connect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::connect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::make_signal_slot_connections()
{
	QObject::connect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::connect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::connect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));
	QObject::connect(
			north_pole_pushbutton, SIGNAL(clicked(bool)),
			this, SLOT(react_north_pole_pushbutton_clicked()));
}
