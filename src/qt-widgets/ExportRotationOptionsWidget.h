/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_EXPORTROTATIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTROTATIONOPTIONSWIDGET_H

#include <QWidget>

#include "ExportRotationOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportRotationOptionsWidget is used to allow the user to change rotations options common
	 * to both *total* and *stage* rotation exports.
	 *
	 * NOTE: This widget is meant to be placed in an exporter-specific @a ExportOptionsWidget.
	 * It doesn't implement the @a ExportOptionsWidget interface.
	 */
	class ExportRotationOptionsWidget :
			public QWidget,
			protected Ui_ExportRotationOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportRotationOptionsWidget using default options.
		 */
		static
		ExportRotationOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportOptionsUtils::ExportRotationOptions &default_export_rotation_options)
		{
			return new ExportRotationOptionsWidget(parent, default_export_rotation_options);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportRotationOptions &
		get_export_rotation_options() const
		{
			return d_export_rotation_options;
		}

	private Q_SLOTS:

		void
		react_identity_radio_button_clicked()
		{
			if (radio_button_indeterminate->isChecked())
			{
				d_export_rotation_options.identity_rotation_format =
					GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_INDETERMINATE;
			}
			else if (radio_button_north_pole->isChecked())
			{
				d_export_rotation_options.identity_rotation_format =
					GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_NORTH_POLE;
			}
		}

		void
		react_euler_pole_format_radio_button_clicked()
		{
			if (radio_button_lat_lon->isChecked())
			{
				d_export_rotation_options.euler_pole_format =
					GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE;
			}
			else if (radio_button_cartesian->isChecked())
			{
				d_export_rotation_options.euler_pole_format =
					GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_CARTESIAN;
			}
		}

	private:

		explicit
		ExportRotationOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportOptionsUtils::ExportRotationOptions &export_rotation_options_) :
			QWidget(parent_),
			d_export_rotation_options(export_rotation_options_)
		{
			setupUi(this);

			//
			// Set the state of the export options widget according to the default export configuration
			// passed to us.
			//

			// The radio buttons are exclusive within their group boxes so only need to check on in each.

			radio_button_indeterminate->setChecked(
					d_export_rotation_options.identity_rotation_format ==
						GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_INDETERMINATE);

			radio_button_lat_lon->setChecked(
					d_export_rotation_options.euler_pole_format ==
						GPlatesGui::ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE);

			make_signal_slot_connections();
		}


		void
		make_signal_slot_connections()
		{
			QObject::connect(
					radio_button_indeterminate,
					SIGNAL(clicked(bool)),
					this,
					SLOT(react_identity_radio_button_clicked()));
			QObject::connect(
					radio_button_north_pole,
					SIGNAL(clicked(bool)),
					this,
					SLOT(react_identity_radio_button_clicked()));
			QObject::connect(
					radio_button_lat_lon,
					SIGNAL(clicked(bool)),
					this,
					SLOT(react_euler_pole_format_radio_button_clicked()));
			QObject::connect(
					radio_button_cartesian,
					SIGNAL(clicked(bool)),
					this,
					SLOT(react_euler_pole_format_radio_button_clicked()));
		}

		GPlatesGui::ExportOptionsUtils::ExportRotationOptions d_export_rotation_options;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTROTATIONOPTIONSWIDGET_H
