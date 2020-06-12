/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_CHOOSEBUILTINPALETTEDIALOG_H
#define GPLATES_QT_WIDGETS_CHOOSEBUILTINPALETTEDIALOG_H

#include <QDialog>

#include "ui_ChooseBuiltinPaletteDialogUi.h"

#include "ColourScaleButton.h"
#include "GPlatesDialog.h"

#include "gui/BuiltinColourPalettes.h"
#include "gui/BuiltinColourPaletteType.h"
#include "gui/RasterColourPalette.h"


namespace GPlatesQtWidgets
{
	class ChooseBuiltinPaletteDialog : 
			public GPlatesDialog,
			protected Ui_ChooseBuiltinPaletteDialog
	{
		Q_OBJECT
		
	public:

		explicit
		ChooseBuiltinPaletteDialog(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters,
				QWidget *parent_ = NULL);
		
	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		builtin_colour_palette_selected(
				const GPlatesGui::BuiltinColourPaletteType &builtin_colour_palette_type);

		void
		builtin_parameters_changed(
				const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters);

	private Q_SLOTS:

		void
		handle_colorbrewer_sequential_classes_changed(
				int value);

		void
		handle_colorbrewer_diverging_classes_changed(
				int value);

		void
		handle_colorbrewer_discrete_check_box_changed(
				int state);

		void
		handle_colorbrewer_invert_check_box_changed(
				int state);

		void
		handle_colour_scale_button_clicked(
				bool checked);

	private:

		void
		add_colour_scale_button(
				ColourScaleButton *colour_scale_button,
				QWidget *colour_scale_button_placeholder);

		GPlatesGui::BuiltinColourPaletteType
		get_builtin_colour_palette_type(
				ColourScaleButton *colour_scale_button);

		GPlatesGui::BuiltinColourPaletteType
		create_colorbrewer_sequential_palette_type(
				GPlatesGui::BuiltinColourPalettes::ColorBrewerSequentialType sequential_type);

		GPlatesGui::BuiltinColourPaletteType
		create_colorbrewer_diverging_palette_type(
				GPlatesGui::BuiltinColourPalettes::ColorBrewerDivergingType diverging_type);

		void
		re_populate_colorbrewer_buttons();


		GPlatesGui::BuiltinColourPaletteType::Parameters d_builtin_parameters;

		// ColorBrewer sequential multi-hue palettes.
		ColourScaleButton *d_BuGn_button;
		ColourScaleButton *d_BuPu_button;
		ColourScaleButton *d_GnBu_button;
		ColourScaleButton *d_OrRd_button;
		ColourScaleButton *d_PuBu_button;
		ColourScaleButton *d_PuBuGn_button;
		ColourScaleButton *d_PuRd_button;
		ColourScaleButton *d_RdPu_button;
		ColourScaleButton *d_YlGn_button;
		ColourScaleButton *d_YlGnBu_button;
		ColourScaleButton *d_YlOrBr_button;
		ColourScaleButton *d_YlOrRd_button;

		// ColorBrewer sequential single hue palettes.
		ColourScaleButton *d_Blues_button;
		ColourScaleButton *d_Greens_button;
		ColourScaleButton *d_Greys_button;
		ColourScaleButton *d_Oranges_button;
		ColourScaleButton *d_Purples_button;
		ColourScaleButton *d_Reds_button;

		// ColorBrewer diverging palettes.
		ColourScaleButton *d_BrBG_button;
		ColourScaleButton *d_PiYG_button;
		ColourScaleButton *d_PRGn_button;
		ColourScaleButton *d_PuOr_button;
		ColourScaleButton *d_RdBu_button;
		ColourScaleButton *d_RdGy_button;
		ColourScaleButton *d_RdYlBu_button;
		ColourScaleButton *d_RdYlGn_button;
		ColourScaleButton *d_Spectral_button;
	};
}

#endif // GPLATES_QT_WIDGETS_CHOOSEBUILTINPALETTEDIALOG_H
