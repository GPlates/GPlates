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

#ifndef GPLATES_QT_WIDGETS_EXPORTFLOWLINEOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTFLOWLINEOPTIONSWIDGET_H

#include <QWidget>

#include "ExportOptionsWidget.h"

#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "gui/ExportFlowlineAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportFlowlineOptionsWidget is used to show export options for
	 * exporting flowlines.
	 *
	 * Currently it's just a placeholder for the @a ExportFileOptionsWidget since no
	 * other options are currently required.
	 */
	class ExportFlowlineOptionsWidget :
			public ExportOptionsWidget
	{
	public:
		/**
		 * Creates a @a ExportFlowlineOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportFlowlineAnimationStrategy::const_configuration_ptr &
						default_export_configuration)
		{
			return new ExportFlowlineOptionsWidget(parent, default_export_configuration);
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

			return GPlatesGui::ExportFlowlineAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportFlowlineAnimationStrategy::Configuration(
							d_export_configuration));
		}

	private:
		explicit
		ExportFlowlineOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportFlowlineAnimationStrategy::const_configuration_ptr &
						default_export_configuration) :
			ExportOptionsWidget(parent_),
			d_export_configuration(*default_export_configuration)
		{
			// Delegate to the export file options widget to collection the file options.
			d_export_file_options_widget =
					ExportFileOptionsWidget::create(
							parent_,
							default_export_configuration->file_options);

			QtWidgetUtils::add_widget_to_placeholder(d_export_file_options_widget, this);
		}


		ExportFileOptionsWidget *d_export_file_options_widget;
		GPlatesGui::ExportFlowlineAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTFLOWLINEOPTIONSWIDGET_H
