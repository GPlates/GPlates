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
 
#ifndef GPLATES_QTWIDGETS_CONFIGURETEXTOVERLAYDIALOG_H
#define GPLATES_QTWIDGETS_CONFIGURETEXTOVERLAYDIALOG_H

#include <QDialog>

#include "ConfigureTextOverlayDialogUi.h"

#include "GPlatesDialog.h"


namespace GPlatesGui
{
	class TextOverlaySettings;
}

namespace GPlatesQtWidgets
{
	class ChooseColourButton;
	class ChooseFontButton;

	class ConfigureTextOverlayDialog : 
			public GPlatesDialog,
			protected Ui_ConfigureTextOverlayDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		ConfigureTextOverlayDialog(
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
				GPlatesGui::TextOverlaySettings &settings);

	private:

		void
		populate(
				const GPlatesGui::TextOverlaySettings &settings);

		void
		save(
				GPlatesGui::TextOverlaySettings &settings);

		ChooseColourButton *d_colour_button;
		ChooseFontButton *d_font_button;
	};
}

#endif  // GPLATES_QTWIDGETS_CONFIGURETEXTOVERLAYDIALOG_H
