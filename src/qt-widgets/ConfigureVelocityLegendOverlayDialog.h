/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2017 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_CONFIGUREVELOCITYLEGENDOVERLAYDIALOG_H
#define GPLATES_QTWIDGETS_CONFIGUREVELOCITYLEGENDOVERLAYDIALOG_H

#include <QDialog>
#include <QToolButton>

#include "gui/Colour.h"

#include "ui_ConfigureVelocityLegendOverlayDialogUi.h"

#include "GPlatesDialog.h"


namespace GPlatesGui
{
	class VelocityLegendOverlaySettings;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{

	class ChooseFontButton;
	class InformationDialog;
	class VisualLayersComboBox;

	/**
	 * @brief The ColourButton class
	 * The ChooseColourButton class (used elsewhere in GPlates) uses the static function getColour which resets
	 * the Alpha value each time it's called.This version of a colour button instantiates a QColorDialog which
	 * allows setting of an alpha value prior to opening the dialog.
	 */
	class ColourButton :
			public QToolButton
	{
		Q_OBJECT

	public:

		explicit
		ColourButton(
				QWidget *parent_ = NULL);

		/**
		 * Set the colour.
		 *
		 * Note: This emits the 'colour_changed' signal if the colour changed.
		 */
		void
		set_colour(
				const GPlatesGui::Colour &colour);

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

	Q_SIGNALS:

		//! Emitted if user changes colour via GUI or if @a set_colour is explicitly called.
		void
		colour_changed(
				GPlatesQtWidgets::ColourButton &);

	private Q_SLOTS:

		void
		handle_clicked();



	private:

		GPlatesGui::Colour d_colour;
	};


	class ConfigureVelocityLegendOverlayDialog :
			public GPlatesDialog,
			protected Ui_ConfigureVelocityLegendOverlayDialog
	{
		Q_OBJECT
		
	public:

		explicit
		ConfigureVelocityLegendOverlayDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		/**
		 * Shows the dialog modal to allow the user to modify the text overlay settings
		 * passed in as a mutable reference, @a settings.
		 *
		 * If the user clicks Cancel, @a settings is not modified.
		 *
		 * Returns QDialog::Accepted or QDialog::Rejected.
		 */
		int
		exec(
				GPlatesGui::VelocityLegendOverlaySettings &settings);


	private Q_SLOTS:

		void
		handle_radio_buttons_checked();


	private:
		using GPlatesDialog::exec;  // We're hiding QDialog::exec()


		/**
		 * @brief populate -  fill the dialog's widgets from the values in @param settings.
		 */
		void
		populate(
				const GPlatesGui::VelocityLegendOverlaySettings &settings);

		/**
		 * @brief save - fill @param settings with values from the widgets.
		 */
		void
		save(
				GPlatesGui::VelocityLegendOverlaySettings &settings);

		GPlatesQtWidgets::ColourButton *d_scale_text_colour_button;
		GPlatesQtWidgets::ColourButton *d_arrow_colour_button;
		GPlatesQtWidgets::ColourButton *d_background_colour_button;

		ChooseFontButton *d_scale_text_font_button;

		VisualLayersComboBox* d_visual_layers_combo_box;

		InformationDialog *d_fixed_scale_help_dialog;
		static const QString s_fixed_scale_text;
		InformationDialog *d_max_arrow_length_help_dialog;
		static const QString s_max_arrow_length_text;

	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGUREVELOCITYLEGENDOVERLAYDIALOG_H
