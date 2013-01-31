/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
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

#include "ExportVelocityOptionsWidget.h"

#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"


GPlatesQtWidgets::ExportVelocityOptionsWidget::ExportVelocityOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr &default_export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*default_export_configuration),
	d_export_file_options_widget(NULL)
{
	setupUi(this);

	// Delegate to the export file options widget to collect the file options.
	d_export_file_options_widget =
			ExportFileOptionsWidget::create(
					parent_,
					default_export_configuration->file_options);
	QtWidgetUtils::add_widget_to_placeholder(
			d_export_file_options_widget,
			widget_file_options);

	make_signal_slot_connections();

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	// Only the GMT file format is interested in the velocity vector output format.
	if (d_export_configuration.file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT)
	{
		if (d_export_configuration.velocity_vector_format == GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_3D)
		{
			velocity_vector_3D_radio_button->setChecked(true);
		}
		else if (d_export_configuration.velocity_vector_format == GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_COLAT_LON)
		{
			velocity_vector_colat_lon_radio_button->setChecked(true);
		}
		else
		{
			velocity_vector_magnitude_angle_radio_button->setChecked(true);
		}
	}
	else
	{
		velocity_vector_format_group_box->hide();
	}

	// Write a description depending on the file format and velocity vector format.
	update_output_description_label();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportVelocityOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the export file options from the export file options widget.
	d_export_configuration.file_options = d_export_file_options_widget->get_export_file_options();

	d_export_configuration.set_filename_template(filename_template);

	return GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportVelocityAnimationStrategy::Configuration(
					d_export_configuration));
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			velocity_vector_3D_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_radio_button_toggled(bool)));
	QObject::connect(
			velocity_vector_colat_lon_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_radio_button_toggled(bool)));
	QObject::connect(
			velocity_vector_magnitude_angle_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_radio_button_toggled(bool)));
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_radio_button_toggled(
		bool checked)
{
	// Determine the file format.
	if (velocity_vector_3D_radio_button->isChecked())
	{
		d_export_configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_3D;
	}
	else if (velocity_vector_colat_lon_radio_button->isChecked())
	{
		d_export_configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_COLAT_LON;
	}
	else
	{
		d_export_configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_MAGNITUDE_ANGLE;
	}

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::update_output_description_label()
{
	// Write a description depending on the file format and velocity vector format.
	switch (d_export_configuration.file_format)
	{
	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GPML:
		output_description_label->setText(" Velocities will be exported in (Colatitude, Longitude) format.");
		break;

	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT:
		switch (d_export_configuration.velocity_vector_format)
		{
		case GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_3D:
			output_description_label->setText(
					" Velocities will be exported as:\n"
					"   domain_point_lon  domain_point_lat  velocity_x  velocity_y  velocity_z  plate_id");
			break;

		case GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_COLAT_LON:
			output_description_label->setText(
					" Velocities will be exported as:\n"
					"   domain_point_lon  domain_point_lat  velocity_colat  velocity_lon  plate_id");
			break;

		case GPlatesFileIO::MultiPointVectorFieldExport::VELOCITY_VECTOR_MAGNITUDE_ANGLE:
			output_description_label->setText(
					" Velocities will be exported as:\n"
					"   domain_point_lon  domain_point_lat  velocity_magnitude  velocity_angle  plate_id");
			break;

		default:
			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}
}
