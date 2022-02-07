/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 Geological Survey of Norway
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <QDebug>

#include "CalculateReconstructionPoleDialog.h"

#include "InsertVGPReconstructionPoleDialog.h"
#include "QtWidgetUtils.h"
#include "ReconstructionPoleWidget.h"

#include "app-logic/PalaeomagUtils.h"

#include "gui/FeatureFocus.h"

#include "maths/GreatCircleArc.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::CalculateReconstructionPoleDialog::CalculateReconstructionPoleDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	GPlatesDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_dialog_ptr(new InsertVGPReconstructionPoleDialog(view_state_, parent_)),
	d_reconstruction_pole_widget_ptr(new ReconstructionPoleWidget(this)),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_feature_focus(view_state_.get_feature_focus())
{
	setupUi(this);

	main_buttonbox->button(QDialogButtonBox::Save)->setText(tr("&Insert Pole in Rotation Model"));

	QtWidgetUtils::add_widget_to_placeholder(
			d_reconstruction_pole_widget_ptr,
			groupbox_recon_pole);

	QObject::connect(
			button_calculate,
			SIGNAL(clicked()),
			this,
			SLOT(handle_calculate()));	
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(clicked(QAbstractButton *)),
			this,
			SLOT(handle_button_clicked(QAbstractButton *)));
	QObject::connect(
			&d_feature_focus,
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(handle_feature_focus_changed()));

	update_buttons();

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::CalculateReconstructionPoleDialog::handle_calculate()
{

	using namespace GPlatesMaths;

	// Grab the lat lon from the VGP boxes.
	double vgp_lat = spinbox_vgp_lat->value();
	double vgp_lon = spinbox_vgp_lon->value();
	
	
	LatLonPoint llp(vgp_lat,vgp_lon);
	PointOnSphere vgp_pole = make_point_on_sphere(llp);
	
	// The dialog settings should prevent a north or south pole being entered here.
	if ((vgp_pole == PointOnSphere::north_pole) ||
		(vgp_pole == PointOnSphere::south_pole))
	{
		// We can't perform any meaningful calculation if the user has entered
		// the north or south pole as input. 
		
		// We could do something fancy like shade the input fields red.
		d_reconstruction_pole.reset(ReconstructionPole(
			spinbox_plateid->value(),
			spinbox_age->value(),
			0, /* zero latitude */
			0,
			0,
			0));

		d_reconstruction_pole_widget_ptr->set_fields(*d_reconstruction_pole);		
		return;
	}
	
	boost::optional<PointOnSphere> geographic_pole;
	if (radio_north->isChecked())
	{
		geographic_pole.reset(PointOnSphere::north_pole);
	}
	else
	{
		geographic_pole.reset(PointOnSphere::south_pole);
	}
	
	const Rotation rotation = Rotation::create(vgp_pole, *geographic_pole);
	const Real angle = rotation.angle();
	
	QLocale locale_;
	// The latitude of llp should be zero, by the way.
	LatLonPoint recon_pole_llp = make_lat_lon_point(PointOnSphere(rotation.axis()));
	
#if 0	
	qDebug() << "lat: " << recon_pole_llp.latitude();
	qDebug() << "lon: " << recon_pole_llp.longitude();
	qDebug() << "angle: " << radiansToDegrees(angle).dval();
	qDebug();	
#endif		

	d_reconstruction_pole.reset(ReconstructionPole(
		spinbox_plateid->value(),
		spinbox_age->value(),
		0, /* zero latitude */
		recon_pole_llp.longitude(),
		GPlatesMaths::convert_rad_to_deg(angle).dval(),
		0));
		
	d_reconstruction_pole_widget_ptr->set_fields(*d_reconstruction_pole);
	
	update_buttons();

}


void
GPlatesQtWidgets::CalculateReconstructionPoleDialog::handle_button_clicked(
		QAbstractButton *button)
{
	QDialogButtonBox::StandardButton button_enum = main_buttonbox->standardButton(button);
	if (button_enum == QDialogButtonBox::Save)
	{
		if (d_reconstruction_pole)
		{
			d_dialog_ptr->setup(*d_reconstruction_pole);
		}
		d_dialog_ptr->show();
	}
}


void
GPlatesQtWidgets::CalculateReconstructionPoleDialog::update_buttons()
{
	//button_apply->setEnabled(d_reconstruction_pole);
	
	// Keep the apply button disabled for now. 
	main_buttonbox->button(QDialogButtonBox::Save)->setEnabled(false);
}

void
GPlatesQtWidgets::CalculateReconstructionPoleDialog::fill_found_fields_from_feature_focus()
{
	if (!d_feature_focus.is_valid())
	{
		return;
	}

	GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder finder;
	finder.visit_feature(d_feature_focus.focused_feature());

	if (!finder.is_vgp_feature())
	{
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> pole_point = finder.get_vgp_point();

	if (pole_point)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(**pole_point);
		spinbox_vgp_lat->setValue(llp.latitude());
		spinbox_vgp_lon->setValue(llp.longitude());
	}

	boost::optional<GPlatesModel::integer_plate_id_type> plate_id = finder.get_plate_id();

	if (plate_id)
	{
		spinbox_plateid->setValue(*plate_id);
	}

	boost::optional<double> age = finder.get_age();

	if (age)
	{
		spinbox_age->setValue(*age);
	}
}

void
GPlatesQtWidgets::CalculateReconstructionPoleDialog::handle_feature_focus_changed()
{
	fill_found_fields_from_feature_focus();
}