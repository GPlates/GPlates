/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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
 
#include "ConfigureCanvasToolGeometryRenderParametersDialog.h"

#include "ChooseColourButton.h"
#include "QtWidgetUtils.h"

#include "view-operations/RenderedGeometryParameters.h"


GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::ConfigureCanvasToolGeometryRenderParametersDialog(
		GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_rendered_geometry_parameters(rendered_geometry_parameters),
	d_focused_feature_clicked_geometry_colour_button(new ChooseColourButton(this)),
	d_topology_focus_colour_button(new ChooseColourButton(this)),
	d_topology_sections_colour_button(new ChooseColourButton(this))
{
	setupUi(this);

	QtWidgetUtils::add_widget_to_placeholder(
			d_focused_feature_clicked_geometry_colour_button,
			focused_feature_clicked_geometry_colour_button_placeholder_widget);
	focused_feature_clicked_geometry_colour_label->setBuddy(d_focused_feature_clicked_geometry_colour_button);
	d_focused_feature_clicked_geometry_colour_button->set_colour(
			rendered_geometry_parameters.get_choose_feature_tool_clicked_geometry_of_focused_feature_colour());

	focused_feature_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_choose_feature_tool_point_size_hint());
	focused_feature_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_choose_feature_tool_line_width_hint());

	QtWidgetUtils::add_widget_to_placeholder(
			d_topology_focus_colour_button,
			topology_focus_colour_button_placeholder_widget);
	topology_focus_colour_label->setBuddy(d_topology_focus_colour_button);
	d_topology_focus_colour_button->set_colour(
			rendered_geometry_parameters.get_topology_tool_focused_geometry_colour());

	topology_focus_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_point_size_hint());
	topology_focus_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_line_width_hint());

	QtWidgetUtils::add_widget_to_placeholder(
			d_topology_sections_colour_button,
			topology_sections_colour_button_placeholder_widget);
	topology_sections_colour_label->setBuddy(d_topology_sections_colour_button);
	d_topology_sections_colour_button->set_colour(
			rendered_geometry_parameters.get_topology_tool_topological_sections_colour());

	topology_sections_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_point_size_hint());
	topology_sections_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_line_width_hint());

	reconstruction_layer_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_point_size_hint());
	reconstruction_layer_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_line_width_hint());
	reconstruction_layer_topology_size_multiplier_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_topology_size_multiplier());

	QObject::connect(
			d_focused_feature_clicked_geometry_colour_button,
			SIGNAL(colour_changed(GPlatesQtWidgets::ChooseColourButton &)),
			this,
			SLOT(react_focused_feature_clicked_geometry_colour_changed()));
	QObject::connect(
			focused_feature_point_size_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_focused_feature_point_size_hint_spinbox_value_changed(double)));
	QObject::connect(
			focused_feature_line_width_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_focused_feature_line_width_hint_spinbox_value_changed(double)));

	QObject::connect(
			d_topology_focus_colour_button,
			SIGNAL(colour_changed(GPlatesQtWidgets::ChooseColourButton &)),
			this,
			SLOT(react_topology_focus_colour_changed()));
	QObject::connect(
			topology_focus_point_size_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_topology_focus_point_size_hint_spinbox_value_changed(double)));
	QObject::connect(
			topology_focus_line_width_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_topology_focus_line_width_hint_spinbox_value_changed(double)));
	QObject::connect(
			d_topology_sections_colour_button,
			SIGNAL(colour_changed(GPlatesQtWidgets::ChooseColourButton &)),
			this,
			SLOT(react_topology_sections_colour_changed()));
	QObject::connect(
			topology_sections_point_size_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_topology_sections_point_size_hint_spinbox_value_changed(double)));
	QObject::connect(
			topology_sections_line_width_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_topology_sections_line_width_hint_spinbox_value_changed(double)));

	QObject::connect(
			reconstruction_layer_point_size_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_reconstruction_layer_point_size_hint_spinbox_value_changed(double)));
	QObject::connect(
			reconstruction_layer_line_width_hint_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_reconstruction_layer_line_width_hint_spinbox_value_changed(double)));
	QObject::connect(
			reconstruction_layer_topology_size_multiplier_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_reconstruction_layer_topology_size_multiplier_spinbox_value_changed(double)));

	// Also update our GUI when the render geometry parameters change.
	QObject::connect(
			&d_rendered_geometry_parameters,
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(handle_rendered_geometry_parameters_changed()));

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_focused_feature_clicked_geometry_colour_changed()
{
	d_rendered_geometry_parameters.set_choose_feature_tool_clicked_geometry_of_focused_feature_colour(
			d_focused_feature_clicked_geometry_colour_button->get_colour());
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_focused_feature_point_size_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_choose_feature_tool_point_size_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_focused_feature_line_width_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_choose_feature_tool_line_width_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_focus_colour_changed()
{
	d_rendered_geometry_parameters.set_topology_tool_focused_geometry_colour(
			d_topology_focus_colour_button->get_colour());
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_focus_point_size_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_topology_tool_focused_geometry_point_size_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_focus_line_width_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_topology_tool_focused_geometry_line_width_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_sections_colour_changed()
{
	d_rendered_geometry_parameters.set_topology_tool_topological_sections_colour(
			d_topology_sections_colour_button->get_colour());
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_sections_point_size_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_topology_tool_topological_sections_point_size_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_topology_sections_line_width_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_topology_tool_topological_sections_line_width_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_reconstruction_layer_point_size_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_reconstruction_layer_point_size_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_reconstruction_layer_line_width_hint_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_reconstruction_layer_line_width_hint(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_reconstruction_layer_topology_size_multiplier_spinbox_value_changed(
		double value)
{
	d_rendered_geometry_parameters.set_reconstruction_layer_topology_size_multiplier(value);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::handle_rendered_geometry_parameters_changed()
{
	// Note: Calling 'ChooseColourButton::set_colour()' will only emit a signal if the colour changes and
	// 'QDoubleSpinBox::setValue()' will only emit signal if value changed.
	// So we shouldn't get infinite recursion.

	d_focused_feature_clicked_geometry_colour_button->set_colour(
			d_rendered_geometry_parameters.get_choose_feature_tool_clicked_geometry_of_focused_feature_colour());

	focused_feature_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_choose_feature_tool_point_size_hint());

	focused_feature_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_choose_feature_tool_line_width_hint());

	d_topology_focus_colour_button->set_colour(
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_colour());

	topology_focus_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_point_size_hint());

	topology_focus_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_focused_geometry_line_width_hint());

	d_topology_sections_colour_button->set_colour(
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_colour());

	topology_sections_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_point_size_hint());

	topology_sections_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_topology_tool_topological_sections_line_width_hint());

	reconstruction_layer_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_point_size_hint());

	reconstruction_layer_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_line_width_hint());

	reconstruction_layer_topology_size_multiplier_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_topology_size_multiplier());
}
