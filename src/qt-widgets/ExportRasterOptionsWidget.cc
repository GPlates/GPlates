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

#include <QVBoxLayout>
#include <QWidget>

#include "ExportRasterOptionsWidget.h"

#include "QtWidgetUtils.h"


GPlatesQtWidgets::ExportRasterOptionsWidget::ExportRasterOptionsWidget(
		QWidget *parent_,
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_image_resolution_options_widget(NULL),
	d_export_configuration(*export_configuration)
{
	QVBoxLayout *widget_layout = new QVBoxLayout(this);
	widget_layout->setContentsMargins(0, 0, 0, 0);

	// Delegate to the export image resolution options widget to collect the image resolution options.
	d_export_image_resolution_options_widget =
			ExportImageResolutionOptionsWidget::create(
					parent_,
					export_animation_context,
					export_configuration->image_resolution_options);
	widget_layout->addWidget(d_export_image_resolution_options_widget);
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportRasterOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	d_export_configuration.set_filename_template(filename_template);

	// Get the export image resolution options from the export image resolution options widget.
	d_export_configuration.image_resolution_options =
			d_export_image_resolution_options_widget->get_export_image_resolution_options();

	return GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportRasterAnimationStrategy::Configuration(
					d_export_configuration));
}
