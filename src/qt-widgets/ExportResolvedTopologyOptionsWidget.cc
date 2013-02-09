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

#include "ExportResolvedTopologyOptionsWidget.h"

#include "DatelineWrapOptionsWidget.h"
#include "ExportFileOptionsWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::ExportResolvedTopologyOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportResolvedTopologyAnimationStrategy::const_configuration_ptr &
				export_configuration,
		bool configure_dateline_wrapping) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*export_configuration),
	d_dateline_wrap_options_widget(NULL),
	d_export_file_options_widget(NULL)
{
	setupUi(this);

	if (configure_dateline_wrapping)
	{
		d_dateline_wrap_options_widget = new DatelineWrapOptionsWidget(
				this,
				d_export_configuration.wrap_to_dateline);
		QtWidgetUtils::add_widget_to_placeholder(
				d_dateline_wrap_options_widget,
				widget_shapefile_dateline_wrap);
	}

	// Delegate to the export file options widget to collect the file options.
	d_export_file_options_widget =
			ExportFileOptionsWidget::create(
					parent_,
					export_configuration->file_options);
	QtWidgetUtils::add_widget_to_placeholder(
			d_export_file_options_widget,
			widget_file_options);

	//
	// Set the state of the export options widget according to the default export configuration passed to us.
	//

	// Topological lines.
	export_resolved_lines_checkbox->setCheckState(
			d_export_configuration.export_topological_lines
			? Qt::Checked
			: Qt::Unchecked);

	// Topological polygons.
	export_resolved_polygons_checkbox->setCheckState(
			d_export_configuration.export_topological_polygons
			? Qt::Checked
			: Qt::Unchecked);

	make_signal_slot_connections();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the export file options from the export file options widget.
	d_export_configuration.file_options = d_export_file_options_widget->get_export_file_options();

	// Get the dateline wrapping options if they've been configured to allow the user to edit them.
	if (d_dateline_wrap_options_widget)
	{
		d_export_configuration.wrap_to_dateline = d_dateline_wrap_options_widget->get_wrap_to_dateline();
	}

	d_export_configuration.set_filename_template(filename_template);

	return GPlatesGui::ExportResolvedTopologyAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportResolvedTopologyAnimationStrategy::Configuration(
					d_export_configuration));
}


void
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			export_resolved_lines_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			export_resolved_polygons_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
}


void
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::react_check_box_state_changed(
		int state)
{
	d_export_configuration.export_topological_lines = export_resolved_lines_checkbox->isChecked();
	d_export_configuration.export_topological_polygons = export_resolved_polygons_checkbox->isChecked();
}
