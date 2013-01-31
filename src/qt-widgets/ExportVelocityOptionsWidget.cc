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

#include "ExportVelocityOptionsWidget.h"

#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ExportVelocityOptionsWidget::ExportVelocityOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr &default_export_configuration) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*default_export_configuration),
	d_export_file_options_widget(NULL)
{
	setupUi(this);

	// Delegate to the export file options widget to collect the file options.
	d_export_file_options_widget =
			ExportFileOptionsWidget::create(
					parent_,
					default_export_configuration->file_options);
	QtWidgetUtils::add_widget_to_placeholder(
			d_export_file_options_widget,
			widget_file_options);

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	make_signal_slot_connections();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportVelocityOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the export file options from the export file options widget.
	d_export_configuration.file_options = d_export_file_options_widget->get_export_file_options();

	d_export_configuration.set_filename_template(filename_template);

	return GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportVelocityAnimationStrategy::Configuration(
					d_export_configuration));
}


void
GPlatesQtWidgets::ExportVelocityOptionsWidget::make_signal_slot_connections()
{
}
