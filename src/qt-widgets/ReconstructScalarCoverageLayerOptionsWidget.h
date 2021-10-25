/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_RECONSTRUCTSCALARCOVERAGELAYEROPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_RECONSTRUCTSCALARCOVERAGELAYEROPTIONSWIDGET_H

#include <utility>

#include "ReconstructScalarCoverageLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"

#include "app-logic/Layer.h"

#include "gui/BuiltinColourPaletteType.h"
#include "gui/RasterColourPalette.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class RemappedColourPaletteWidget;
	class ViewportWindow;

	/**
	 * ReconstructScalarCoverageLayerOptionsWidget is used to show additional options for
	 * reconstructing scalar coverages (geometries with scalars) in the visual layers widget.
	 */
	class ReconstructScalarCoverageLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_ReconstructScalarCoverageLayerOptionsWidget
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

	private Q_SLOTS:

		void
		handle_scalar_type_combobox_activated(
				const QString &text);

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

		void
		handle_builtin_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_colour_palette_type);

		void
		handle_builtin_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters);

		void
		handle_palette_range_check_box_changed(
				int state);

		void
		handle_palette_min_line_editing_finished(
				double value);

		void
		handle_palette_max_line_editing_finished(
				double value);

		void
		handle_palette_range_restore_min_max_button_clicked();

		void
		handle_palette_range_restore_mean_deviation_button_clicked();

		void
		handle_palette_range_restore_mean_deviation_spinbox_changed(
				double value);

	private:

		ReconstructScalarCoverageLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		std::pair<double, double>
		get_scalar_min_max(
				GPlatesAppLogic::Layer &layer) const;

		std::pair<double, double>
		get_scalar_mean_std_dev(
				GPlatesAppLogic::Layer &layer) const;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		OpenFileDialog d_open_file_dialog;
		RemappedColourPaletteWidget *d_colour_palette_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

	};
}

#endif // GPLATES_QT_WIDGETS_RECONSTRUCTSCALARCOVERAGELAYEROPTIONSWIDGET_H
