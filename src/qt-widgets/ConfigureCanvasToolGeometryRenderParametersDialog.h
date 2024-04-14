/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_CONFIGURECANVASTOOLGEOMETRYRENDERPARAMETERSDIALOG_H
#define GPLATES_QTWIDGETS_CONFIGURECANVASTOOLGEOMETRYRENDERPARAMETERSDIALOG_H

#include <QDialog>

#include "ui_ConfigureCanvasToolGeometryRenderParametersDialogUi.h"

#include "GPlatesDialog.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryParameters;
}

namespace GPlatesQtWidgets
{
	class ChooseColourButton;

	class ConfigureCanvasToolGeometryRenderParametersDialog : 
			public GPlatesDialog,
			protected Ui_ConfigureCanvasToolGeometryRenderParametersDialog
	{
		Q_OBJECT
		
	public:

		explicit
		ConfigureCanvasToolGeometryRenderParametersDialog(
				GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters,
				QWidget *parent_ = NULL);
		
	private Q_SLOTS:

		void
		react_focused_feature_clicked_geometry_colour_changed();

		void
		react_focused_feature_point_size_hint_spinbox_value_changed(
				double value);

		void
		react_focused_feature_line_width_hint_spinbox_value_changed(
				double value);

		void
		react_topology_focus_colour_changed();

		void
		react_topology_focus_point_size_hint_spinbox_value_changed(
				double value);

		void
		react_topology_focus_line_width_hint_spinbox_value_changed(
				double value);

		void
		react_topology_sections_colour_changed();

		void
		react_topology_sections_point_size_hint_spinbox_value_changed(
				double value);

		void
		react_topology_sections_line_width_hint_spinbox_value_changed(
				double value);

		void
		react_reconstruction_layer_point_size_hint_spinbox_value_changed(
				double value);

		void
		react_reconstruction_layer_line_width_hint_spinbox_value_changed(
				double value);

		void
		react_reconstruction_layer_topology_size_multiplier_spinbox_value_changed(
				double value);

		void
		handle_rendered_geometry_parameters_changed();

	private:

		GPlatesViewOperations::RenderedGeometryParameters &d_rendered_geometry_parameters;

		ChooseColourButton *d_focused_feature_clicked_geometry_colour_button;
		ChooseColourButton *d_topology_focus_colour_button;
		ChooseColourButton *d_topology_sections_colour_button;
	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGURECANVASTOOLGEOMETRYRENDERPARAMETERSDIALOG_H
