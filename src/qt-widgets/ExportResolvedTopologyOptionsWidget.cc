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
#include "InformationDialog.h"
#include "QtWidgetUtils.h"


namespace GPlatesQtWidgets
{
	namespace
	{
		const QString HELP_EXPORT_TOPOLOGICAL_LINE_SUB_SEGMENTS_DIALOG_TITLE = QObject::tr(
				"Topological line sub-segments");
		const QString HELP_EXPORT_TOPOLOGICAL_LINE_SUB_SEGMENTS_DIALOG_TEXT = QObject::tr(
				"<html><body>\n"
				"<h3>Export topological line sub-segments</h3>"

				"<p>The boundary of a single topological polygon or network consists of multiple "
				"boundary segments. And a single boundary segment can be from a regular geometry or a "
				"topological line. In the latter case the boundary segment in turn consists of "
				"multiple sub-segments (because a topological line itself consists of segments).</p>"

				"<p>If this option is enabled then the individual sub-segment geometries of a "
				"boundary segment are exported. This also means the feature properties of each individual "
				"sub-segment are exported (such as plate ID and feature type), not the properties of the "
				"topological line. For topological lines this option results in more accurate plate IDs but "
				"the individual sub-segment feature types may not match the feature type of the topological line "
				"(depending on how they were built). Note that this only applies to boundary segments associated "
				"with topological lines. A boundary segment associated with a regular (non-topological) line "
				"is always exported as a single geometry/feature.</p>"

				"<p>Conversely if this option is <em>not</em> enabled then a single geometry is exported for each "
				"boundary segment geometry (as if its individual sub-segment geometries were joined together). "
				"This also means the feature properties of the topological line itself are exported (such as plate ID "
				"and feature type), not the properties of the individual sub-segment features. For topological lines "
				"this option has the advantage of a single geometry per boundary segment, but the plate ID is probably "
				"not meaningful (since the individual sub-segments likely have varying plate IDs).</p>"
				"</body></html>\n");
	}
}


GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::ExportResolvedTopologyOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportResolvedTopologyAnimationStrategy::const_configuration_ptr &
				export_configuration,
		bool configure_dateline_wrapping) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*export_configuration),
	d_dateline_wrap_options_widget(NULL),
	d_export_file_options_widget(NULL),
	d_help_export_topological_line_sub_segments_dialog(
			new InformationDialog(
					HELP_EXPORT_TOPOLOGICAL_LINE_SUB_SEGMENTS_DIALOG_TEXT,
					HELP_EXPORT_TOPOLOGICAL_LINE_SUB_SEGMENTS_DIALOG_TITLE,
					this))
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

	// Topological networks.
	export_resolved_networks_checkbox->setCheckState(
			d_export_configuration.export_topological_networks
			? Qt::Checked
			: Qt::Unchecked);

	// Topological sections.
	export_resolved_boundary_segments_checkbox->setCheckState(
			d_export_configuration.export_topological_sections
			? Qt::Checked
			: Qt::Unchecked);

	// Topological line sub-segments.
	export_topological_line_sub_segments_checkbox->setCheckState(
			d_export_configuration.export_topological_line_sub_segments
			? Qt::Checked
			: Qt::Unchecked);

	// Enable polygons options only if exporting resolved polygons or networks.
	polygon_options->setEnabled(
			d_export_configuration.export_topological_polygons ||
				d_export_configuration.export_topological_networks);
	force_polygon_orientation_checkbox->setCheckState(
			d_export_configuration.force_polygon_orientation
			? Qt::Checked
			: Qt::Unchecked);
	// Show shapefile and non-shapefile options based on the file format.
	non_shapefile_polygon_options->setVisible(
			d_export_configuration.file_format !=
				GPlatesGui::ExportResolvedTopologyAnimationStrategy::Configuration::SHAPEFILE);
	shapefile_polygon_options_label->setVisible(
			d_export_configuration.file_format ==
				GPlatesGui::ExportResolvedTopologyAnimationStrategy::Configuration::SHAPEFILE);
	// Enable polygon orientation combobox only if forcing polygon orientation.
	polygon_orientation_combobox->setEnabled(
			static_cast<bool>(d_export_configuration.force_polygon_orientation));
	// Add polygon orientation combobox values.
	polygon_orientation_combobox->insertItem(
			GPlatesMaths::PolygonOrientation::CLOCKWISE,
			tr("Clockwise"));
	polygon_orientation_combobox->insertItem(
			GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE,
			tr("Counter-clockwise"));
	// Set the current polygon orientation combobox value.
	polygon_orientation_combobox->setCurrentIndex(
			d_export_configuration.force_polygon_orientation
			? d_export_configuration.force_polygon_orientation.get()
			// Default to clockwise if polygon orientation not specified (not forced)...
			: GPlatesMaths::PolygonOrientation::CLOCKWISE);

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
			SLOT(react_export_resolved_geometry_check_box_state_changed(int)));
	QObject::connect(
			export_resolved_polygons_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_export_resolved_geometry_check_box_state_changed(int)));
	QObject::connect(
			export_resolved_networks_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_export_resolved_geometry_check_box_state_changed(int)));
	QObject::connect(
			export_resolved_boundary_segments_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_export_resolved_geometry_check_box_state_changed(int)));
	QObject::connect(
			export_topological_line_sub_segments_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_export_resolved_geometry_check_box_state_changed(int)));
	QObject::connect(
			force_polygon_orientation_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_force_polygon_orientation_check_box_state_changed(int)));
	QObject::connect(
			polygon_orientation_combobox,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(react_polygon_orientation_combobox_state_changed(int)));

	// Connect the help dialog.
	QObject::connect(
			push_button_help_export_topological_line_sub_segments, SIGNAL(clicked()),
			d_help_export_topological_line_sub_segments_dialog, SLOT(show()));
}


void
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::react_export_resolved_geometry_check_box_state_changed(
		int state)
{
	d_export_configuration.export_topological_lines = export_resolved_lines_checkbox->isChecked();
	d_export_configuration.export_topological_polygons = export_resolved_polygons_checkbox->isChecked();
	d_export_configuration.export_topological_networks = export_resolved_networks_checkbox->isChecked();
	d_export_configuration.export_topological_sections = export_resolved_boundary_segments_checkbox->isChecked();
	d_export_configuration.export_topological_line_sub_segments = export_topological_line_sub_segments_checkbox->isChecked();

	// Enable polygons options only if exporting resolved polygons or networks.
	polygon_options->setEnabled(
			export_resolved_polygons_checkbox->isChecked() ||
				export_resolved_networks_checkbox->isChecked());
}


void
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::react_force_polygon_orientation_check_box_state_changed(
		int state)
{
	// Set polygon orientation only if forcing polygon orientation.
	if (force_polygon_orientation_checkbox->isChecked())
	{
		switch (polygon_orientation_combobox->currentIndex())
		{
		case GPlatesMaths::PolygonOrientation::CLOCKWISE:
			d_export_configuration.force_polygon_orientation = GPlatesMaths::PolygonOrientation::CLOCKWISE;
			break;

		case GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE:
			d_export_configuration.force_polygon_orientation = GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE;
			break;
		}
	}
	else
	{
		d_export_configuration.force_polygon_orientation = boost::none;
	}

	// Enable polygon orientation combobox only if forcing polygon orientation.
	polygon_orientation_combobox->setEnabled(force_polygon_orientation_checkbox->isChecked());
}


void
GPlatesQtWidgets::ExportResolvedTopologyOptionsWidget::react_polygon_orientation_combobox_state_changed(
		int index)
{
	// Set polygon orientation only if forcing polygon orientation.
	// Note: Shouldn't be able to get here anyway if not forcing (since then combobox should be disabled).
	if (force_polygon_orientation_checkbox->isChecked())
	{
		switch (index)
		{
		case GPlatesMaths::PolygonOrientation::CLOCKWISE:
			d_export_configuration.force_polygon_orientation = GPlatesMaths::PolygonOrientation::CLOCKWISE;
			break;

		case GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE:
			d_export_configuration.force_polygon_orientation = GPlatesMaths::PolygonOrientation::COUNTERCLOCKWISE;
			break;
		}
	}
}
