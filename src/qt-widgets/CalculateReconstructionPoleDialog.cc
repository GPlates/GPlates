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
#include <QDebug>

#include "CalculateReconstructionPoleDialog.h"

#include "InsertVGPReconstructionPoleDialog.h"
#include "QtWidgetUtils.h"
#include "ReconstructionPoleWidget.h"

#include "maths/GreatCircleArc.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

#include "presentation/ViewState.h"



GPlatesQtWidgets::CalculateReconstructionPoleDialog::CalculateReconstructionPoleDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_dialog_ptr(new InsertVGPReconstructionPoleDialog(view_state_, parent_)),
	d_reconstruction_pole_widget_ptr(new ReconstructionPoleWidget(this)),
	d_application_state_ptr(&view_state_.get_application_state())
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

	update_buttons();
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
	
	Rotation rotation = Rotation::create(vgp_pole, *geographic_pole);
	Real angle = rotation.angle();
	// Restrict angle to between 0 and 90 degrees. 
	if (angle > GPlatesMaths::HALF_PI)
	{
		angle = GPlatesMaths::PI-angle;
	}
	
	QLocale locale_;
	// The latitude of llp should be zero, by the way.
	LatLonPoint recon_pole_llp = make_lat_lon_point(PointOnSphere(rotation.axis()));
	
#if 0	
	qDebug() << "lat: " << recon_pole_llp.latitude();
	qDebug() << "lon: " << recon_pole_llp.longitude();
	qDebug() << "angle: " << radiansToDegrees(angle).dval();
	qDebug();	
#endif	

#if 0
	line_rot_lat->setText(locale_.toString(recon_pole_llp.latitude(), 'f', 2));
	line_rot_lon->setText(locale_.toString(recon_pole_llp.longitude(), 'f', 2));
	line_rot_angle->setText(locale_.toString(radiansToDegrees(angle).dval(), 'f', 2));
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

