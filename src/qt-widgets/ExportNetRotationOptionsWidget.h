/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#ifndef GPLATES_QT_WIDGETS_EXPORTNETROTATIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTNETROTATIONOPTIONSWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "ExportOptionsWidget.h"

#if 0
#include "DatelineWrapOptionsWidget.h"
#include "ExportFileOptionsWidget.h"
#endif

#include "VelocityMethodWidget.h"
#include "QtWidgetUtils.h"

#include "gui/ExportNetRotationAnimationStrategy.h"
#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportNetRotationOptionsWidget is used to show export options for
	 * exporting net rotations.
	 */
	class ExportNetRotationOptionsWidget :
			public ExportOptionsWidget
	{
		Q_OBJECT
	public:
		/**
		 * Creates a @a ExportNetRotationOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportNetRotationAnimationStrategy::const_configuration_ptr &
						export_configuration)
		{
			return new ExportNetRotationOptionsWidget(
					parent, export_configuration);
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

			d_export_configuration.d_options =
				GPlatesGui::ExportOptionsUtils::ExportNetRotationOptions(
					d_velocity_method_widget->delta_time(),
					d_velocity_method_widget->velocity_method());


			return GPlatesGui::ExportNetRotationAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportNetRotationAnimationStrategy::Configuration(
							d_export_configuration));
		}


	private:
		explicit
		ExportNetRotationOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportNetRotationAnimationStrategy::const_configuration_ptr &
						export_configuration) :
			ExportOptionsWidget(parent_),
			d_velocity_method_widget(NULL),
			d_export_configuration(*export_configuration)
		{
			QVBoxLayout *widget_layout = new QVBoxLayout(this);
			widget_layout->setContentsMargins(0, 0, 0, 0);

			d_velocity_method_widget = new VelocityMethodWidget(false,this);
			widget_layout->addWidget(d_velocity_method_widget);

			d_velocity_method_widget->set_delta_time(d_export_configuration.d_options.delta_time);
			d_velocity_method_widget->set_velocity_method(d_export_configuration.d_options.velocity_method);

		}

		VelocityMethodWidget *d_velocity_method_widget;
		GPlatesGui::ExportNetRotationAnimationStrategy::Configuration d_export_configuration;

	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTNETROTATIONOPTIONSWIDGET_H
