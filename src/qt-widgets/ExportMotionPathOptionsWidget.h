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

#ifndef GPLATES_QT_WIDGETS_EXPORTMOTIONPATHOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTMOTIONPATHOPTIONSWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "ExportOptionsWidget.h"

#include "DatelineWrapOptionsWidget.h"
#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "gui/ExportMotionPathAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportMotionPathOptionsWidget is used to show export options for
	 * exporting motion paths.
	 */
	class ExportMotionPathOptionsWidget :
			public ExportOptionsWidget
	{
	public:
		/**
		 * Creates a @a ExportMotionPathOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportMotionPathAnimationStrategy::const_configuration_ptr &
						export_configuration,
				bool configure_dateline_wrapping)
		{
			return new ExportMotionPathOptionsWidget(
					parent, export_configuration, configure_dateline_wrapping);
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

			// Get the export file options from the export file options widget.
			d_export_configuration.file_options =
					d_export_file_options_widget->get_export_file_options();
			if (d_dateline_wrap_options_widget)
			{
				d_export_configuration.wrap_to_dateline =
						d_dateline_wrap_options_widget->get_wrap_to_dateline();
			}

			return GPlatesGui::ExportMotionPathAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportMotionPathAnimationStrategy::Configuration(
							d_export_configuration));
		}

	private:
		explicit
		ExportMotionPathOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportMotionPathAnimationStrategy::const_configuration_ptr &
						export_configuration,
				bool configure_dateline_wrapping) :
			ExportOptionsWidget(parent_),
			d_dateline_wrap_options_widget(NULL),
			d_export_file_options_widget(NULL),
			d_export_configuration(*export_configuration)
		{
			QVBoxLayout *widget_layout = new QVBoxLayout(this);
			widget_layout->setContentsMargins(0, 0, 0, 0);

			if (configure_dateline_wrapping)
			{
				d_dateline_wrap_options_widget = new DatelineWrapOptionsWidget(
						this,
						d_export_configuration.wrap_to_dateline);
				widget_layout->addWidget(d_dateline_wrap_options_widget);
			}

			// Delegate to the export file options widget to collection the file options.
			d_export_file_options_widget =
					ExportFileOptionsWidget::create(
							parent_,
							export_configuration->file_options);
			widget_layout->addWidget(d_export_file_options_widget);
		}


		DatelineWrapOptionsWidget *d_dateline_wrap_options_widget;
		ExportFileOptionsWidget *d_export_file_options_widget;
		GPlatesGui::ExportMotionPathAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTMOTIONPATHOPTIONSWIDGET_H
