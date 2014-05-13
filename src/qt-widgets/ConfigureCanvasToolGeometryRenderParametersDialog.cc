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
	d_focused_feature_non_clicked_geometry_colour_button(new ChooseColourButton(this))
{
	setupUi(this);

	topology_tools_groupbox->hide();

	QtWidgetUtils::add_widget_to_placeholder(
			d_focused_feature_clicked_geometry_colour_button,
			focused_feature_clicked_geometry_colour_button_placeholder_widget);
	focused_feature_clicked_geometry_colour_label->setBuddy(d_focused_feature_clicked_geometry_colour_button);
	d_focused_feature_clicked_geometry_colour_button->set_colour(
			rendered_geometry_parameters.get_reconstruction_layer_clicked_geometry_of_focused_feature_colour());

	QtWidgetUtils::add_widget_to_placeholder(
			d_focused_feature_non_clicked_geometry_colour_button,
			focused_feature_non_clicked_geometry_colour_button_placeholder_widget);
	focused_feature_non_clicked_geometry_colour_label->setBuddy(d_focused_feature_non_clicked_geometry_colour_button);
	d_focused_feature_non_clicked_geometry_colour_button->set_colour(
			rendered_geometry_parameters.get_reconstruction_layer_non_clicked_geometry_of_focused_feature_colour());
	// It's probably a bit too difficult for the user to understand what this is so we'll just hide it
	// - most features only have a single geometry property so this doesn't apply in most situations.
	focused_feature_non_clicked_geometry_colour_label->hide();
	focused_feature_non_clicked_geometry_colour_widget->hide();

	reconstruction_layer_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_point_size_hint());
	reconstruction_layer_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_line_width_hint());

	QObject::connect(
			d_focused_feature_clicked_geometry_colour_button,
			SIGNAL(colour_changed(GPlatesQtWidgets::ChooseColourButton &)),
			this,
			SLOT(react_focused_feature_clicked_geometry_colour_changed()));
	QObject::connect(
			d_focused_feature_non_clicked_geometry_colour_button,
			SIGNAL(colour_changed(GPlatesQtWidgets::ChooseColourButton &)),
			this,
			SLOT(react_focused_feature_non_clicked_geometry_colour_changed()));
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

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_focused_feature_clicked_geometry_colour_changed()
{
	d_rendered_geometry_parameters.set_reconstruction_layer_clicked_geometry_of_focused_feature_colour(
			d_focused_feature_clicked_geometry_colour_button->get_colour());
}


void
GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog::react_focused_feature_non_clicked_geometry_colour_changed()
{
	d_rendered_geometry_parameters.set_reconstruction_layer_non_clicked_geometry_of_focused_feature_colour(
			d_focused_feature_non_clicked_geometry_colour_button->get_colour());
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
