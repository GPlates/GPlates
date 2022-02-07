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

	include_dilatation_strain_check_box->setChecked(d_export_configuration->include_dilatation_strain);
	include_dilatation_strain_rate_check_box->setChecked(d_export_configuration->include_dilatation_strain_rate);
	include_second_invariant_strain_rate_check_box->setChecked(d_export_configuration->include_second_invariant_strain_rate);

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
	}
	else
	{
		gmt_format_options->hide();
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
	QObject::connect(
			include_dilatation_strain_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_include_dilatation_strain_check_box_clicked()));
	QObject::connect(
			include_dilatation_strain_rate_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_include_dilatation_strain_rate_check_box_clicked()));
	QObject::connect(
			include_second_invariant_strain_rate_check_box,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_include_second_invariant_check_box_clicked()));

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

	//
	// GPML format connections.
	//

}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_gmt_domain_point_format_radio_button_toggled(
		bool checked)
{
	// All radio buttons in the group are connected to the same slot (this method).
	// Hence there will be *two* calls to this slot even though there's only *one* user action (clicking a button).
	// One slot call is for the button that is toggled off and the other slot call for the button toggled on.
	// However we handle all buttons in one call to this slot so it should only be called once.
	// So we only look at one signal.
	// We arbitrarily choose the signal from the button toggled *on* (*off* would have worked fine too).
	if (!checked)
	{
		return;
	}

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
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_include_dilatation_strain_check_box_clicked()
{
	d_export_configuration->include_dilatation_strain = include_dilatation_strain_check_box->isChecked();

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_include_dilatation_strain_rate_check_box_clicked()
{
	d_export_configuration->include_dilatation_strain_rate = include_dilatation_strain_rate_check_box->isChecked();

	update_output_description_label();
}


void
GPlatesQtWidgets::ExportScalarCoverageOptionsWidget::react_include_second_invariant_check_box_clicked()
{
	d_export_configuration->include_second_invariant_strain_rate  = include_second_invariant_strain_rate_check_box->isChecked();

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

			output_description = tr("Scalar coverages containing visible scalar values will be exported.\n");

			if (configuration.include_dilatation_strain ||
				configuration.include_dilatation_strain_rate ||
				configuration.include_second_invariant_strain_rate)
			{
				output_description += tr("Also deformation will be exported as:\n");

				if (configuration.include_dilatation_strain)
				{
					output_description += tr("  DilatationStrain\n");
				}

				if (configuration.include_dilatation_strain_rate)
				{
					output_description += tr("  DilatationStrainRate\n");
				}

				if (configuration.include_second_invariant_strain_rate)
				{
					output_description += tr("  TotalStrainRate\n");
				}
			}
		}
		break;

	case GPlatesGui::ExportScalarCoverageAnimationStrategy::Configuration::GMT:
		{
			// Throws bad_cast if fails.
			GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &configuration =
					dynamic_cast<GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration &>(
							*d_export_configuration);

			output_description = tr("Scalar values will be exported as:\n");

			if (configuration.domain_point_format ==
				GPlatesGui::ExportScalarCoverageAnimationStrategy::GMTConfiguration::LON_LAT)
			{
				output_description += tr("  longitude  latitude");
			}
			else
			{
				output_description += tr("  latitude  longitude");
			}

			if (configuration.include_dilatation_strain)
			{
				output_description += tr("  dilatation_strain");
			}

			if (configuration.include_dilatation_strain_rate)
			{
				output_description += tr("  dilatation_strain_rate");
			}

			if (configuration.include_second_invariant_strain_rate)
			{
				output_description += tr("  total_strain_rate");
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
