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

#ifndef GPLATES_QT_WIDGETS_EXPORTRECONSTRUCTEDGEOMETRYOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTRECONSTRUCTEDGEOMETRYOPTIONSWIDGET_H

#include <QVBoxLayout>
#include <QWidget>

#include "ExportOptionsWidget.h"

#include "DatelineWrapOptionsWidget.h"
#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"

#include "gui/ExportReconstructedGeometryAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportReconstructedGeometryOptionsWidget is used to show export options for
	 * exporting reconstructed geometries.
	 *
	 * Currently it's just a placeholder for the @a ExportFileOptionsWidget since no
	 * other options are currently required.
	 */
	class ExportReconstructedGeometryOptionsWidget :
			public ExportOptionsWidget
	{
	public:
		/**
		 * Creates a @a ExportReconstructedGeometryOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr &
						default_export_configuration,
				bool configure_dateline_wrapping)
		{
			return new ExportReconstructedGeometryOptionsWidget(
					parent, default_export_configuration, configure_dateline_wrapping);
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
			d_export_configuration.wrap_to_dateline =
					d_dateline_wrap_options_widget->get_wrap_to_dateline();

			return GPlatesGui::ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr(
					new GPlatesGui::ExportReconstructedGeometryAnimationStrategy::Configuration(
							d_export_configuration));
		}

	private:
		explicit
		ExportReconstructedGeometryOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportReconstructedGeometryAnimationStrategy::const_configuration_ptr &
						default_export_configuration,
				bool configure_dateline_wrapping) :
			ExportOptionsWidget(parent_),
			d_export_configuration(*default_export_configuration)
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
							default_export_configuration->file_options);
			widget_layout->addWidget(d_export_file_options_widget);
		}


		DatelineWrapOptionsWidget *d_dateline_wrap_options_widget;
		ExportFileOptionsWidget *d_export_file_options_widget;
		GPlatesGui::ExportReconstructedGeometryAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTRECONSTRUCTEDGEOMETRYOPTIONSWIDGET_H
