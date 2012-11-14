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

#ifndef GPLATES_QT_WIDGETS_EXPORTTOTALROTATIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTTOTALROTATIONOPTIONSWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "ExportOptionsWidget.h"
#include "ExportRotationOptionsWidget.h"

#include "QtWidgetUtils.h"

#include "gui/ExportTotalRotationAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportTotalRotationOptionsWidget is used to show export options for
	 * exporting total rotations (including equivalent and relative rotations).
	 */
	class ExportTotalRotationOptionsWidget :
			public ExportOptionsWidget
	{
	public:
		/**
		 * Creates a @a ExportTotalRotationOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportTotalRotationAnimationStrategy::const_configuration_ptr &
						default_export_configuration)
		{
			return new ExportTotalRotationOptionsWidget(
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

			return GPlatesGui::ExportTotalRotationAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportTotalRotationAnimationStrategy::Configuration(
							d_export_configuration));
		}

	private:

		explicit
		ExportTotalRotationOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportTotalRotationAnimationStrategy::const_configuration_ptr &
						default_export_configuration) :
			ExportOptionsWidget(parent_),
			d_export_rotation_options_widget(NULL),
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
		}


		ExportRotationOptionsWidget *d_export_rotation_options_widget;
		GPlatesGui::ExportTotalRotationAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTTOTALROTATIONOPTIONSWIDGET_H
