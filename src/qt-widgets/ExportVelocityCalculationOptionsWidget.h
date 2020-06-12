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

#ifndef GPLATES_QT_WIDGETS_EXPORTVELOCITYCALCULATIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTVELOCITYCALCULATIONOPTIONSWIDGET_H

#include <boost/static_assert.hpp>
#include <QWidget>

#include "ui_ExportVelocityCalculationOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportVelocityCalculationOptionsWidget is used to allow the user to change the velocity
	 * delta-time interval and type, and also enable smoothing of velocities near plate boundaries
	 * (and to adjust any smoothing options).
	 *
	 * NOTE: This widget is meant to be placed in an exporter-specific @a ExportOptionsWidget.
	 * It doesn't implement the @a ExportOptionsWidget interface.
	 */
	class ExportVelocityCalculationOptionsWidget :
			public QWidget,
			protected Ui_ExportVelocityCalculationOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportVelocityCalculationOptionsWidget using default options.
		 */
		static
		ExportVelocityCalculationOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportOptionsUtils::ExportVelocityCalculationOptions &
					default_export_velocity_calculation_options)
		{
			return new ExportVelocityCalculationOptionsWidget(parent, default_export_velocity_calculation_options);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportVelocityCalculationOptions &
		get_export_velocity_calculation_options() const
		{
			return d_export_velocity_calculation_options;
		}

	private Q_SLOTS:

		void
		handle_velocity_delta_time_type_button(
				bool checked)
		{
			if (radio_t_plus_dt_to_t->isChecked())
			{
				d_export_velocity_calculation_options.delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T;
			}
			if (radio_t_to_t_minus_dt->isChecked())
			{
				d_export_velocity_calculation_options.delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T;
			}
			if (radio_t_plus_dt_2_to_t_minus_dt_2->isChecked())
			{
				d_export_velocity_calculation_options.delta_time_type = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T;
			}

			// Update this source code if more enumeration values have been added (or removed).
			BOOST_STATIC_ASSERT(GPlatesAppLogic::VelocityDeltaTime::NUM_TYPES == 3);
		}

		void
		handle_velocity_delta_time_value_changed(
				double value)
		{
			d_export_velocity_calculation_options.delta_time = value;
		}

		void
		react_velocity_smoothing_check_box_changed()
		{
			d_export_velocity_calculation_options.is_boundary_smoothing_enabled =
					velocity_smoothing_check_box->isChecked();

			// Only display velocity smoothing controls if velocity smoothing is enabled.
			velocity_smoothing_controls->setVisible(
					d_export_velocity_calculation_options.is_boundary_smoothing_enabled);
		}

		void
		react_velocity_smoothing_distance_spinbox_changed(
				double value)
		{
			d_export_velocity_calculation_options.boundary_smoothing_angular_half_extent_degrees = value;
		}

		void
		react_exclude_smoothing_in_deforming_regions_check_box_changed()
		{
			d_export_velocity_calculation_options.exclude_deforming_regions =
					exclude_smoothing_in_deforming_regions_check_box->isChecked();
		}

	private:

		explicit
		ExportVelocityCalculationOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportOptionsUtils::ExportVelocityCalculationOptions &export_velocity_calculation_options_) :
			QWidget(parent_),
			d_export_velocity_calculation_options(export_velocity_calculation_options_)
		{
			setupUi(this);

			//
			// Set the state of the export options widget according to the default export configuration passed to us.
			//

			radio_t_plus_dt_to_t->setChecked(
					d_export_velocity_calculation_options.delta_time_type == GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T);
			radio_t_to_t_minus_dt->setChecked(
					d_export_velocity_calculation_options.delta_time_type == GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T);
			radio_t_plus_dt_2_to_t_minus_dt_2->setChecked(
					d_export_velocity_calculation_options.delta_time_type == GPlatesAppLogic::VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T);
			velocity_delta_time_spinbox->setValue(d_export_velocity_calculation_options.delta_time);

			velocity_smoothing_check_box->setCheckState(
					d_export_velocity_calculation_options.is_boundary_smoothing_enabled
					? Qt::Checked
					: Qt::Unchecked);
			velocity_smoothing_distance_spinbox->setValue(
					d_export_velocity_calculation_options.boundary_smoothing_angular_half_extent_degrees);
			exclude_smoothing_in_deforming_regions_check_box->setCheckState(
					d_export_velocity_calculation_options.exclude_deforming_regions
					? Qt::Checked
					: Qt::Unchecked);

			// Display velocity smoothing controls if smoothing is enabled.
			velocity_smoothing_controls->setVisible(
					d_export_velocity_calculation_options.is_boundary_smoothing_enabled);

			make_signal_slot_connections();
		}


		void
		make_signal_slot_connections()
		{
			QObject::connect(
					radio_t_plus_dt_to_t, SIGNAL(toggled(bool)),
					this, SLOT(handle_velocity_delta_time_type_button(bool)));
			QObject::connect(
					radio_t_to_t_minus_dt, SIGNAL(toggled(bool)),
					this, SLOT(handle_velocity_delta_time_type_button(bool)));
			QObject::connect(
					radio_t_plus_dt_2_to_t_minus_dt_2, SIGNAL(toggled(bool)),
					this, SLOT(handle_velocity_delta_time_type_button(bool)));
			QObject::connect(
					velocity_delta_time_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_velocity_delta_time_value_changed(double)));

			QObject::connect(
					velocity_smoothing_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(react_velocity_smoothing_check_box_changed()));
			QObject::connect(
					velocity_smoothing_distance_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(react_velocity_smoothing_distance_spinbox_changed(double)));
			QObject::connect(
					exclude_smoothing_in_deforming_regions_check_box, SIGNAL(stateChanged(int)),
					this, SLOT(react_exclude_smoothing_in_deforming_regions_check_box_changed()));
		}

		GPlatesGui::ExportOptionsUtils::ExportVelocityCalculationOptions d_export_velocity_calculation_options;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTVELOCITYCALCULATIONOPTIONSWIDGET_H
