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

#ifndef GPLATES_QT_WIDGETS_REMAPPEDCOLOURPALETTEWIDGET_H
#define GPLATES_QT_WIDGETS_REMAPPEDCOLOURPALETTEWIDGET_H

#include <QWidget>

#include "RemappedColourPaletteWidgetUi.h"

#include "gui/BuiltinColourPaletteType.h"

#include "presentation/RemappedColourPaletteParameters.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ChooseBuiltinPaletteDialog;
	class ColourScaleWidget;
	class FriendlyLineEdit;
	class ViewportWindow;


	/**
	 * A widget containing a colour palette and options to remap the palette range according
	 * to min/max or mean/standard-deviation.
	 */
	class RemappedColourPaletteWidget :
			public QWidget,
			protected Ui_RemappedColourPaletteWidget
	{
		Q_OBJECT

	public:

		/**
		 * If @a extra_widget is specified then it is added to the 'extra_placeholder_widget',
		 * otherwise that area is not visible.
		 */
		explicit
		RemappedColourPaletteWidget(
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_,
				QWidget *extra_widget = NULL);


		/**
		 * Set parameters to configure the state of the widget.
		 *
		 * Note that this does not emit any signals.
		 *
		 * This does not set the min/max limits of the lower/upper spinboxes though.
		 */
		void
		set_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &parameters);


	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		select_palette_filename_button_clicked();

		void
		use_default_palette_button_clicked();

		void
		builtin_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_colour_palette_type);

		void
		builtin_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters);

		void
		range_check_box_changed(
				int);

		void
		min_line_editing_finished(
				double);

		void
		max_line_editing_finished(
				double);

		void
		range_restore_min_max_button_clicked();

		void
		range_restore_mean_deviation_button_clicked();

		void
		range_restore_mean_deviation_spinbox_changed(
				double value);

	private Q_SLOTS:

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

		void
		open_choose_builtin_palette_dialog();

		void
		handle_builtin_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_colour_palette_type);

		void
		handle_builtin_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters);

		void
		handle_range_check_box_changed(
				int);

		void
		handle_min_line_editing_finished();

		void
		handle_max_line_editing_finished();

		void
		handle_range_restore_min_max_button_clicked();

		void
		handle_range_restore_mean_deviation_button_clicked();

		void
		handle_range_restore_mean_deviation_spinbox_changed(
				double value);

	private:

		ViewportWindow *d_viewport_window;
		FriendlyLineEdit *d_palette_name_lineedit;
		ChooseBuiltinPaletteDialog *d_choose_builtin_palette_dialog;
		ColourScaleWidget *d_colour_scale_widget;

		//! The built-in colour palette parameters for use in the built-in palette dialog.
		GPlatesGui::BuiltinColourPaletteType::Parameters d_builtin_colour_palette_parameters;
	};
}

#endif // GPLATES_QT_WIDGETS_REMAPPEDCOLOURPALETTEWIDGET_H
