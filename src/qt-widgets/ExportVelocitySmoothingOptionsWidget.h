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

#ifndef GPLATES_QT_WIDGETS_EXPORTVELOCITYSMOOTHINGOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTVELOCITYSMOOTHINGOPTIONSWIDGET_H

#include <QWidget>

#include "ExportVelocitySmoothingOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportVelocitySmoothingOptionsWidget is used to allow the user to enable smoothing of
	 * velocities near plate boundaries and to adjust any smoothing options.
	 *
	 * NOTE: This widget is meant to be placed in an exporter-specific @a ExportOptionsWidget.
	 * It doesn't implement the @a ExportOptionsWidget interface.
	 */
	class ExportVelocitySmoothingOptionsWidget :
			public QWidget,
			protected Ui_ExportVelocitySmoothingOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportVelocitySmoothingOptionsWidget using default options.
		 */
		static
		ExportVelocitySmoothingOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportOptionsUtils::ExportVelocitySmoothingOptions &
					default_export_velocity_smoothing_options)
		{
			return new ExportVelocitySmoothingOptionsWidget(parent, default_export_velocity_smoothing_options);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportVelocitySmoothingOptions &
		get_export_velocity_smoothing_options() const
		{
			return d_export_velocity_smoothing_options;
		}

	private Q_SLOTS:

		void
		react_velocity_smoothing_check_box_changed()
		{
			d_export_velocity_smoothing_options.is_boundary_smoothing_enabled =
					velocity_smoothing_check_box->isChecked();

			// Only enable velocity smoothing controls if velocity smoothing is enabled.
			velocity_smoothing_controls->setEnabled(
					d_export_velocity_smoothing_options.is_boundary_smoothing_enabled);
		}

		void
		react_velocity_smoothing_distance_spinbox_changed(
				double value)
		{
			d_export_velocity_smoothing_options.boundary_smoothing_angular_half_extent_degrees = value;
		}

		void
		react_exclude_smoothing_in_deforming_regions_check_box_changed()
		{
			d_export_velocity_smoothing_options.exclude_deforming_regions =
					exclude_smoothing_in_deforming_regions_check_box->isChecked();
		}

	private:

		explicit
		ExportVelocitySmoothingOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportOptionsUtils::ExportVelocitySmoothingOptions &export_velocity_smoothing_options_) :
			QWidget(parent_),
			d_export_velocity_smoothing_options(export_velocity_smoothing_options_)
		{
			setupUi(this);

			//
			// Set the state of the export options widget according to the default export configuration
			// passed to us.
			//

			velocity_smoothing_check_box->setCheckState(
					d_export_velocity_smoothing_options.is_boundary_smoothing_enabled
					? Qt::Checked
					: Qt::Unchecked);
			velocity_smoothing_distance_spinbox->setValue(
					d_export_velocity_smoothing_options.boundary_smoothing_angular_half_extent_degrees);
			exclude_smoothing_in_deforming_regions_check_box->setCheckState(
					d_export_velocity_smoothing_options.exclude_deforming_regions
					? Qt::Checked
					: Qt::Unchecked);

			// Enable velocity smoothing controls if smoothing is enabled.
			velocity_smoothing_controls->setEnabled(
					d_export_velocity_smoothing_options.is_boundary_smoothing_enabled);

			make_signal_slot_connections();
		}


		void
		make_signal_slot_connections()
		{
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

		GPlatesGui::ExportOptionsUtils::ExportVelocitySmoothingOptions d_export_velocity_smoothing_options;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTVELOCITYSMOOTHINGOPTIONSWIDGET_H
