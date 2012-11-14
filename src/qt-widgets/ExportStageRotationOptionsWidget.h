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

#ifndef GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONOPTIONSWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "ExportOptionsWidget.h"
#include "ExportRotationOptionsWidget.h"
#include "ExportStageRotationOnlyOptionsWidget.h"

#include "QtWidgetUtils.h"

#include "gui/ExportStageRotationAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportStageRotationOptionsWidget is used to show export options for
	 * exporting stage rotations (including equivalent and relative rotations).
	 */
	class ExportStageRotationOptionsWidget :
			public ExportOptionsWidget
	{
	public:
		/**
		 * Creates a @a ExportStageRotationOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportStageRotationAnimationStrategy::const_configuration_ptr &
						default_export_configuration)
		{
			return new ExportStageRotationOptionsWidget(
					parent,
					default_export_configuration);
		}


		/**
		 * Collects the options specified by the user and
		 * returns them as an export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template)
		{
			d_export_configuration.set_filename_template(filename_template);

			// Get the export rotation options from the export rotation options widget.
			d_export_configuration.rotation_options =
					d_export_rotation_options_widget->get_export_rotation_options();

			// Get the export *stage* rotation options from the export *stage* rotation options widget.
			d_export_configuration.stage_rotation_options =
					d_export_stage_rotation_only_options_widget->get_export_stage_rotation_options();

			return GPlatesGui::ExportStageRotationAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportStageRotationAnimationStrategy::Configuration(
							d_export_configuration));
		}

	private:

		explicit
		ExportStageRotationOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportStageRotationAnimationStrategy::const_configuration_ptr &
						default_export_configuration) :
			ExportOptionsWidget(parent_),
			d_export_rotation_options_widget(NULL),
			d_export_stage_rotation_only_options_widget(NULL),
			d_export_configuration(*default_export_configuration)
		{
			QVBoxLayout *widget_layout = new QVBoxLayout(this);
			widget_layout->setContentsMargins(0, 0, 0, 0);

			// Delegate to the export rotation options widget to collect the rotation options.
			d_export_rotation_options_widget =
					ExportRotationOptionsWidget::create(
							parent_,
							default_export_configuration->rotation_options);
			widget_layout->addWidget(d_export_rotation_options_widget);

			// Delegate to the export *stage* rotation options widget to collect the *stage* rotation options.
			d_export_stage_rotation_only_options_widget =
					ExportStageRotationOnlyOptionsWidget::create(
							parent_,
							default_export_configuration->stage_rotation_options);
			widget_layout->addWidget(d_export_stage_rotation_only_options_widget);
		}


		ExportRotationOptionsWidget *d_export_rotation_options_widget;
		ExportStageRotationOnlyOptionsWidget *d_export_stage_rotation_only_options_widget;
		GPlatesGui::ExportStageRotationAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTSTAGEROTATIONOPTIONSWIDGET_H
