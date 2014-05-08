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
	d_colour_button(NULL/*new ChooseColourButton(this)*/)
{
	setupUi(this);

// 	QtWidgetUtils::add_widget_to_placeholder(
// 			d_colour_button,
// 			colour_button_placeholder_widget);
// 	colour_label->setBuddy(d_colour_button);

	reconstruction_layer_point_size_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_point_size_hint());
	reconstruction_layer_line_width_hint_spinbox->setValue(
			d_rendered_geometry_parameters.get_reconstruction_layer_line_width_hint());

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
