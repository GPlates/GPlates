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
 
#ifndef GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H

#include <utility>
#include <vector>

#include "ScalarField3DLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class Layer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ColourScaleWidget;
	class FriendlyLineEdit;
	class ReadErrorAccumulationDialog;
	class ViewportWindow;

	/**
	 * ScalarField3DLayerOptionsWidget is used to show additional options for
	 * 3D scalar field layers in the visual layers widget.
	 */
	class ScalarField3DLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_ScalarField3DLayerOptionsWidget
	{
		Q_OBJECT

	public:

		static
		LayerOptionsWidget *
		create(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		virtual
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

		virtual
		const QString &
		get_title();

		~ScalarField3DLayerOptionsWidget();

	private slots:

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

		void
		handle_iso_value_spinbox_changed(
				double value);

		void
		handle_iso_value_slider_changed(
				int value);

		void
		handle_test_variable_spinbox_changed(
				double value);

	private:

		ScalarField3DLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		std::pair<double, double>
		get_scalar_field_min_max(
				GPlatesAppLogic::Layer &layer) const;

		int
		get_slider_iso_value(
				const double &iso_value,
				GPlatesAppLogic::Layer &layer) const;


		/**
		 * The number of QDoubleSpinBox's used for shader test variables.
		 */
		static const unsigned int NUM_SHADER_TEST_VARIABLES = 10;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		FriendlyLineEdit *d_palette_filename_lineedit;
		OpenFileDialog d_open_file_dialog;
		ColourScaleWidget *d_colour_scale_widget;

		std::vector<float> d_shader_test_variables;


		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_SCALARFIELD3DLAYEROPTIONSWIDGET_H
