/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "ExportScalarCoverageOptionsWidget.h"

#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "file-io/ExportTemplateFilenameSequence.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::ExportScalarCoverageOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportScalarCoverageAnimationStrategy::const_configuration_ptr &export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(
			boost::dynamic_pointer_cast<
					GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration>(
							export_configuration->clone())),
	d_export_file_options_widget(ExportFileOptionsWidget::create(parent_, export_configuration->file_options))
{
	setupUi(this);

	QtWidgetUtils::add_widget_to_placeholder(
			d_export_file_options_widget,
			widget_file_options);

	// Make signal/slot connections *before* we set values on the GUI controls.
	make_signal_slot_connections();

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	if (d_export_configuration->file_format == GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration::GMT)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
						*d_export_configuration);

		if (configuration.domain_point_format ==
			GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration::LON_LAT)
		{
			gmt_lon_lat_radio_button->setChecked(true);
		}
		else
		{
			gmt_lat_lon_radio_button->setChecked(true);
		}

		gmt_include_domain_point_check_box->setChecked(configuration.include_domain_point);
		gmt_include_dilatation_rate_check_box->setChecked(configuration.include_dilatation_rate);
		gmt_include_domain_meta_data_check_box->setChecked(configuration.include_domain_meta_data);

		// Disable the domain point format options if we're not exporting domain points.
		gmt_domain_point_format_options->setEnabled(configuration.include_domain_point);
	}
	else
	{
		gmt_format_options->hide();
	}

	if (d_export_configuration->file_format == GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration::GPML)
	{
		// Throws bad_cast if fails.
		const GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &configuration =
				dynamic_cast<const GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &>(
						*d_export_configuration);

		gpml_include_dilatation_rate_check_box->setChecked(configuration.include_dilatation_rate);
	}
	else
	{
		gpml_format_options->hide();
	}

	// Write a description depending on the file format and scalar coverage options.
	update_output_description_label();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the export file options from the export file options widget.
	d_export_configuration->file_options = d_export_file_options_widget->get_export_file_options();

	d_export_configuration->set_filename_template(filename_template);

	return d_export_configuration->clone();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::make_signal_slot_connections()
{
	//
	// GMT format connections.
	//

	QObject::connect(
			gmt_lon_lat_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_domain_point_format_radio_button_toggled(bool)));
	QObject::connect(
			gmt_lat_lon_radio_button,
			SIGNAL(toggled(bool)),
			this,
			SLOT(react_gmt_domain_point_format_radio_button_toggled(bool)));
	QObject::connect(
			gmt_include_domain_point_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_domain_point_check_box_clicked()));
	QObject::connect(
			gmt_include_dilatation_rate_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_dilatation_rate_check_box_clicked()));
	QObject::connect(
			gmt_include_domain_meta_data_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gmt_include_domain_meta_data_check_box_clicked()));

	//
	// GPML format connections.
	//

	QObject::connect(
			gpml_include_dilatation_rate_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_gpml_include_dilatation_rate_check_box_clicked()));
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gmt_domain_point_format_radio_button_toggled(
		bool checked)
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	// Determine the domain point format.
	if (gmt_lon_lat_radio_button->isChecked())
	{
		configuration.domain_point_format = GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration::LON_LAT;
	}
	else
	{
		configuration.domain_point_format = GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration::LAT_LON;
	}

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gmt_include_dilatation_rate_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_dilatation_rate = gmt_include_dilatation_rate_check_box->isChecked();

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gmt_include_domain_point_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_domain_point = gmt_include_domain_point_check_box->isChecked();

	// Disable the domain point format options if we're not exporting domain points.
	gmt_domain_point_format_options->setEnabled(configuration.include_domain_point);

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gmt_include_domain_meta_data_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
					*d_export_configuration);

	configuration.include_domain_meta_data = gmt_include_domain_meta_data_check_box->isChecked();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gpml_include_dilatation_rate_check_box_clicked()
{
	// Throws bad_cast if fails.
	GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &configuration =
			dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &>(
					*d_export_configuration);

	configuration.include_dilatation_rate = gpml_include_dilatation_rate_check_box->isChecked();

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::update_output_description_label()
{
	QString output_description;

	// Write a description depending on the file format and associated options.
	switch (d_export_configuration->file_format)
	{
	case GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration::GPML:
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GpmlConfiguration &>(
							*d_export_configuration);

			output_description = tr("Scalar values will be exported ") +
					(configuration.include_dilatation_rate ? tr("with") : tr("without")) +
					tr(" dilatation rate\n");
		}
		break;

	case GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration::GMT:
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
							*d_export_configuration);

			output_description = tr("Scalar values will be exported as:\n");

			if (configuration.include_domain_point)
			{
				if (configuration.domain_point_format ==
					GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration::LON_LAT)
				{
					output_description += tr("  domain_point_lon  domain_point_lat");
				}
				else
				{
					output_description += tr("  domain_point_lat  domain_point_lon");
				}
			}

			if (configuration.include_dilatation_rate)
			{
				output_description += tr("  dilatation_rate");
			}

			output_description += tr("  scalar");

			output_description += "\n";
		}
		break;

	default:
		// Shouldn't get here.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Set the label text.
	scalar_output_description_label->setText(output_description);
}
