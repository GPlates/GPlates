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

#ifndef GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONONLYOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONONLYOPTIONSWIDGET_H

#include <QWidget>

#include "ExportStageRotationOnlyOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportStageRotationOnlyOptionsWidget is used to allow the user to change rotations options that
	 * *only* appy to *stage* rotation exports (not *total* rotation exports).
	 *
	 * NOTE: This widget is meant to be placed in an exporter-specific @a ExportOptionsWidget.
	 * It doesn't implement the @a ExportOptionsWidget interface.
	 */
	class ExportStageRotationOnlyOptionsWidget :
			public QWidget,
			protected Ui_ExportStageRotationOnlyOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportStageRotationOnlyOptionsWidget using default options.
		 */
		static
		ExportStageRotationOnlyOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportOptionsUtils::ExportStageRotationOptions &default_export_stage_rotation_options_)
		{
			return new ExportStageRotationOnlyOptionsWidget(parent, default_export_stage_rotation_options_);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportStageRotationOptions &
		get_export_stage_rotation_options() const
		{
			return d_export_stage_rotation_options;
		}

	private slots:

		void
		react_time_interval_value_changed(
				double time_interval)
		{
			d_export_stage_rotation_options.time_interval = time_interval;
		}

	private:

		explicit
		ExportStageRotationOnlyOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportOptionsUtils::ExportStageRotationOptions &export_stage_rotation_options_) :
			QWidget(parent_),
			d_export_stage_rotation_options(export_stage_rotation_options_)
		{
			setupUi(this);

			//
			// Set the state of the export options widget according to the default export configuration
			// passed to us.
			//

			double_spin_box_time_interval->setValue(d_export_stage_rotation_options.time_interval);

			make_signal_slot_connections();
		}


		void
		make_signal_slot_connections()
		{
			QObject::connect(
					double_spin_box_time_interval,
					SIGNAL(valueChanged(double)),
					this,
					SLOT(react_time_interval_value_changed(double)));
		}

		GPlatesGui::ExportOptionsUtils::ExportStageRotationOptions d_export_stage_rotation_options;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONONLYOPTIONSWIDGET_H
