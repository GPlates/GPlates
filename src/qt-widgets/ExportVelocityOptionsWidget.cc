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

#include <QMessageBox>

#include "ExportVelocityOptionsWidget.h"

#include "ExportFileOptionsWidget.h"
#include "ExportVelocityCalculationOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "file-io/ExportTemplateFilenameSequence.h"
#include "file-io/MultiPointVectorFieldExport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesQtWidgets::ExportVelocityOptionsWidget::ExportVelocityOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr &export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(
			boost::dynamic_pointer_cast<
					GPlatesGui::ExportVelocityAnimationStrategy::Configuration>(
							export_configuration->clone())),
	d_export_velocity_calculation_options_widget(NULL),
	d_export_file_options_widget(NULL)
{
	setupUi(this);

	// Delegate to the export file options widget to collect the file options.
	// Note that not all formats support this.

	// All velocity layers have velocity calculation options.
	d_export_velocity_calculation_options_widget =
			ExportVelocityCalculationOptionsWidget::create(
					parent_,
					d_export_configuration->velocity_calculation_options);
	QtWidgetUtils::add_widget_to_placeholder(
			d_export_velocity_calculation_options_widget,
			widget_velocity_calculation_options);

	if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GPML)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportVelocityAnimationStrategy::GpmlConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportVelocityAnimationStrategy::GpmlConfiguration &>(
						*d_export_configuration);

		d_export_file_options_widget =
				ExportFileOptionsWidget::create(
						parent_,
						configuration.file_options);
		QtWidgetUtils::add_widget_to_placeholder(
				d_export_file_options_widget,
				widget_file_options);
	}

	if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
						*d_export_configuration);

		d_export_file_options_widget =
				ExportFileOptionsWidget::create(
						parent_,
						configuration.file_options);
		QtWidgetUtils::add_widget_to_placeholder(
				d_export_file_options_widget,
				widget_file_options);
	}

	// Make signal/slot connections *before* we set values on the GUI controls.
	make_signal_slot_connections();

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	// Only the GMT file format is interested in the velocity vector output format.
	if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
						*d_export_configuration);

		if (configuration.velocity_vector_format == GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_3D)
		{
			velocity_vector_3D_radio_button->setChecked(true);
		}
		else if (configuration.velocity_vector_format == GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_COLAT_LON)
		{
			velocity_vector_colat_lon_radio_button->setChecked(true);
		}
		else if (configuration.velocity_vector_format == GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_ANGLE_MAGNITUDE)
		{
			velocity_vector_angle_magnitude_radio_button->setChecked(true);
		}
		else
		{
			velocity_vector_azimuth_magnitude_radio_button->setChecked(true);
		}

		velocity_scale_spin_box->setValue(configuration.velocity_scale);
		velocity_stride_spin_box->setValue(configuration.velocity_stride);

		if (configuration.domain_point_format ==
			GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration::LON_LAT)
		{
			lon_lat_radio_button->setChecked(true);
		}
		else
		{
			lat_lon_radio_button->setChecked(true);
		}

		include_plate_id_check_box->setChecked(configuration.include_plate_id);
		include_domain_point_check_box->setChecked(configuration.include_domain_point);
		include_domain_meta_data_check_box->setChecked(configuration.include_domain_meta_data);

		// Disable the domain point format options if we're not exporting domain points.
		domain_point_format_options->setEnabled(configuration.include_domain_point);
	}
	else
	{
		gmt_format_options->hide();
	}

	// Only Terra text format has a Terra grid filename template option.
	if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_TEXT)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration &>(
						*d_export_configuration);

		// The default filename template.
		terra_grid_filename_template_line_edit->setText(configuration.terra_grid_filename_template);

		// Set the template description label text.
		terra_grid_filename_template_description_label->setText(
				tr("This identifies input Terra grid parameters required for each exported velocity file.\n"
					"Use '%1' to locate the local processor number in the Terra grid file name.\n"
					"Use '%2', '%3' and '%4' to locate the Terra parameters 'mt', 'nt' and 'nd'.\n"
					"Velocities are only exported if matching Terra grid files are already loaded.")
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::PROCESSOR_PLACE_HOLDER)
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::MT_PLACE_HOLDER)
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::NT_PLACE_HOLDER)
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::ND_PLACE_HOLDER));
	}
	else
	{
		terra_grid_filename_template_group_box->hide();
	}

	// Only CitcomS global format has a CitcomS grid filename template option.
	if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::CITCOMS_GLOBAL)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
						*d_export_configuration);

		// The default filename template.
		citcoms_grid_filename_template_line_edit->setText(configuration.citcoms_grid_filename_template);

		// Set the template description label text.
		citcoms_grid_filename_template_description_label->setText(
				tr("This identifies input CitcomS grid parameters required for each exported velocity file.\n"
					"Use '%1' to locate the diamond cap number in the CitcomS grid file name.\n"
					"Use '%2' to locate the diamond density/resolution.\n"
					"Velocities are only exported if matching CitcomS grid files are already loaded.")
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER)
						.arg(GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::DENSITY_PLACE_HOLDER));

		citcoms_gmt_format_check_box->setChecked(configuration.include_gmt_export);
		citcoms_gmt_format_options->setEnabled(configuration.include_gmt_export);

		citcoms_gmt_velocity_scale_spin_box->setValue(configuration.gmt_velocity_scale);
		citcoms_gmt_velocity_stride_spin_box->setValue(configuration.gmt_velocity_stride);
	}
	else
	{
		citcoms_format_options->hide();
	}

	// Write a description depending on the file format and velocity vector format.
	update_output_description_label();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportVelocityOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the export velocity calculation options from the export velocity calculation options widget.
	if (d_export_velocity_calculation_options_widget)
	{
		d_export_configuration->velocity_calculation_options =
				d_export_velocity_calculation_options_widget->get_export_velocity_calculation_options();
	}

	// Get the export file options from the export file options widget, if any.
	if (d_export_file_options_widget)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT ||
					d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GPML,
				GPLATES_ASSERTION_SOURCE);

		if (d_export_configuration->file_format == GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT)
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
							*d_export_configuration);
			configuration.file_options = d_export_file_options_widget->get_export_file_options();
		}
		else
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportVelocityAnimationStrategy::GpmlConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GpmlConfiguration &>(
							*d_export_configuration);
			configuration.file_options = d_export_file_options_widget->get_export_file_options();
		}
	}

	d_export_configuration->set_filename_template(filename_template);

	return d_export_configuration->clone();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::make_signal_slot_connections()
{
	//
	// GMT format connections.
	//

	QObject::connect(
			velocity_vector_3D_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_velocity_vector_format_radio_button_toggled(bool)));
	QObject::connect(
			velocity_vector_colat_lon_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_velocity_vector_format_radio_button_toggled(bool)));
	QObject::connect(
			velocity_vector_angle_magnitude_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_velocity_vector_format_radio_button_toggled(bool)));
	QObject::connect(
			velocity_vector_azimuth_magnitude_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_velocity_vector_format_radio_button_toggled(bool)));
	QObject::connect(
			velocity_scale_spin_box,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_gmt_velocity_scale_spin_box_value_changed(double)));
	QObject::connect(
			velocity_stride_spin_box,
			SIGNAL(valueChanged(int)),
			this,
			SLOT(react_gmt_velocity_stride_spin_box_value_changed(int)));
	QObject::connect(
			lon_lat_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_domain_point_format_radio_button_toggled(bool)));
	QObject::connect(
			lat_lon_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_domain_point_format_radio_button_toggled(bool)));
	QObject::connect(
			include_plate_id_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_plate_id_check_box_clicked()));
	QObject::connect(
			include_domain_point_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_domain_point_check_box_clicked()));
	QObject::connect(
			include_domain_meta_data_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_domain_meta_data_check_box_clicked()));

	//
	// Terra text format connections.
	//

	QObject::connect(
			terra_grid_filename_template_line_edit,
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_terra_grid_filename_template_changed()));

	//
	// CitcomS global format connections.
	//

	QObject::connect(
			citcoms_grid_filename_template_line_edit,
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_citcoms_grid_filename_template_changed()));
	QObject::connect(
			citcoms_gmt_format_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_citcoms_gmt_format_check_box_clicked()));
	QObject::connect(
			citcoms_gmt_velocity_scale_spin_box,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_citcoms_gmt_velocity_scale_spin_box_value_changed(double)));
	QObject::connect(
			citcoms_gmt_velocity_stride_spin_box,
			SIGNAL(valueChanged(int)),
			this,
			SLOT(react_citcoms_gmt_velocity_stride_spin_box_value_changed(int)));
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_velocity_vector_format_radio_button_toggled(
		bool checked)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	// Determine the file format.
	if (velocity_vector_3D_radio_button->isChecked())
	{
		configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_3D;
	}
	else if (velocity_vector_colat_lon_radio_button->isChecked())
	{
		configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_COLAT_LON;
	}
	else if (velocity_vector_angle_magnitude_radio_button->isChecked())
	{
		configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_ANGLE_MAGNITUDE;
	}
	else
	{
		configuration.velocity_vector_format = GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_AZIMUTH_MAGNITUDE;
	}

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_velocity_scale_spin_box_value_changed(
		double value)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.velocity_scale = value;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_velocity_stride_spin_box_value_changed(
		int value)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.velocity_stride = value;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_domain_point_format_radio_button_toggled(
		bool checked)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	// Determine the domain point format.
	if (lon_lat_radio_button->isChecked())
	{
		configuration.domain_point_format = GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration::LON_LAT;
	}
	else
	{
		configuration.domain_point_format = GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration::LAT_LON;
	}

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_include_plate_id_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_plate_id = include_plate_id_check_box->isChecked();

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_include_domain_point_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_domain_point = include_domain_point_check_box->isChecked();

	// Disable the domain point format options if we're not exporting domain points.
	domain_point_format_options->setEnabled(configuration.include_domain_point);

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_gmt_include_domain_meta_data_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_domain_meta_data = include_domain_meta_data_check_box->isChecked();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::handle_terra_grid_filename_template_changed()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration &>(
					*d_export_configuration);

	const QString text = terra_grid_filename_template_line_edit->text();

	// Find occurrence of each Terra parameter in file name template.
	const int mt_pos = text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::MT_PLACE_HOLDER);
	const int nt_pos = text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::NT_PLACE_HOLDER);
	const int nd_pos = text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::ND_PLACE_HOLDER);
	const int np_pos = text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::PROCESSOR_PLACE_HOLDER);

	// Must have one, and only one, occurrence of each Terra parameter placeholders.
	if (text.isEmpty() ||
		mt_pos < 0 ||
		nt_pos < 0 ||
		nd_pos < 0 ||
		np_pos < 0 ||
		text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::MT_PLACE_HOLDER,
			mt_pos+1) >= 0 ||
		text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::NT_PLACE_HOLDER,
			nt_pos+1) >= 0 ||
		text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::ND_PLACE_HOLDER,
			nd_pos+1) >= 0 ||
		text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::PROCESSOR_PLACE_HOLDER,
			np_pos+1) >= 0)
	{
		QMessageBox::warning(
				this,
				tr("Invalid Terra grid file name template"),
				tr("The Terra grid file name template must contain one, and only one, occurrence of each of "
					"'%1', '%2', '%3' and '%4'.")
					.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::MT_PLACE_HOLDER)
					.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::NT_PLACE_HOLDER)
					.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::ND_PLACE_HOLDER)
					.arg(GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::PROCESSOR_PLACE_HOLDER),
				QMessageBox::Ok, QMessageBox::Ok);
		terra_grid_filename_template_line_edit->setText(configuration.terra_grid_filename_template);
		return;
	}

	configuration.terra_grid_filename_template = text;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::handle_citcoms_grid_filename_template_changed()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
					*d_export_configuration);

	const QString text = citcoms_grid_filename_template_line_edit->text();

	// Find occurrence of the CitcomS cap number placeholder in file name template.
	const int cap_number_pos = text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER);

	// Must have one, and only one, occurrence of the CitcomS cap number placeholder.
	// The density placeholder can occur zero or more times since it's not used in the export file name.
	if (text.isEmpty() ||
		cap_number_pos < 0 ||
		text.indexOf(
			GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER,
			cap_number_pos+1) >= 0)
	{
		QMessageBox::warning(
				this,
				tr("Invalid CitcomS grid file name template"),
				tr("The CitcomS grid file name template must contain one, and only one, occurrence '%1'.")
					.arg(GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER),
				QMessageBox::Ok, QMessageBox::Ok);
		citcoms_grid_filename_template_line_edit->setText(configuration.citcoms_grid_filename_template);
		return;
	}

	configuration.citcoms_grid_filename_template = text;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_citcoms_gmt_format_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
					*d_export_configuration);

	configuration.include_gmt_export = citcoms_gmt_format_check_box->isChecked();
	citcoms_gmt_format_options->setEnabled(configuration.include_gmt_export);

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_citcoms_gmt_velocity_scale_spin_box_value_changed(
		double value)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
					*d_export_configuration);

	configuration.gmt_velocity_scale = value;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::react_citcoms_gmt_velocity_stride_spin_box_value_changed(
		int value)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
					*d_export_configuration);

	configuration.gmt_velocity_stride = value;
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::update_output_description_label()
{
	QString output_description;

	// Write a description depending on the file format and velocity vector format.
	switch (d_export_configuration->file_format)
	{
	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GPML:
		output_description = "Velocities will be exported in (Colatitude, Longitude) format.\n";
		break;

	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::GMT:
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration &>(
							*d_export_configuration);

			output_description = tr("Velocities will be exported as:\n");

			if (configuration.include_domain_point)
			{
				if (configuration.domain_point_format ==
					GPlatesGui::ExportVelocityAnimationStrategy::GMTConfiguration::LON_LAT)
				{
					output_description += tr("  domain_point_lon  domain_point_lat");
				}
				else
				{
					output_description += tr("  domain_point_lat  domain_point_lon");
				}
			}

			//
			// NOTE: The velocity vector should be immediately after the domain point (columns 1 and 2) since
			// the GMT psxy '-Sv'/'-SV' options require vector angle/azimuth in column 3 and magnitude in column 4.
			// 
			switch (configuration.velocity_vector_format)
			{
			case GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_3D:
				output_description += tr("  velocity_x  velocity_y  velocity_z");
				break;

			case GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_COLAT_LON:
				output_description += tr("  velocity_colat  velocity_lon");
				break;

			case GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_ANGLE_MAGNITUDE:
				// The GMT psxy '-Sv' option requires angle in column 3 and magnitude in column 4.
				output_description += tr("  velocity_angle  velocity_magnitude");
				break;

			case GPlatesFileIO::MultiPointVectorFieldExport::GMT_VELOCITY_VECTOR_AZIMUTH_MAGNITUDE:
				// The GMT psxy '-SV' option requires azimuth in column 3 and magnitude in column 4.
				output_description += tr("  velocity_azimuth  velocity_magnitude");
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}

			if (configuration.include_plate_id)
			{
				output_description += tr("  plate_id");
			}

			output_description += "\n";
		}
		break;

	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_TEXT:
		output_description =
				tr("'%1' will be replaced by the local processor number in each exported velocity file name.\n"
					"The header lines, beginning with '>', contain Terra grid parameters and age.\n"
					"Then each velocity line contains:\n"
					"  velocity_x  velocity_y  velocity_z\n")
					.arg(GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING);
		break;

	case GPlatesGui::ExportVelocityAnimationStrategy::Configuration::CITCOMS_GLOBAL:
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration &>(
							*d_export_configuration);

			output_description = tr(
					"In each exported velocity file name, '%1'"
					" will be replaced by the diamond cap number.\n")
							.arg(GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING);

			output_description += tr(
					"Each velocity line in a CitcomS file contains:\n"
					"  velocity_colat  velocity_lon\n");

			if (configuration.include_gmt_export)
			{
				// Note the that domain point is lat/lon and not the default GMT format lon/lat.
				// This is the way the "convert_meshes_gpml_to_citcoms.py" script exports.
				output_description += tr(
						"Each velocity line in a GMT ('.xy') file contains:\n"
						"  domain_point_lat  domain_point_lon  velocity_azimuth  velocity_magnitude\n");
			}
		}
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Add a description of the velocity (magnitude) units.
	output_description += "\nNote: velocities are in cm/year.\n";

	// Set the label text.
	velocity_output_description_label->setText(output_description);
}
