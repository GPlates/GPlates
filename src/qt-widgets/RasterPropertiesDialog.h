/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
 * (as "SetRasterSurfaceExtentDialog.h")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "RasterPropertiesDialog.h")
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
 
#ifndef GPLATES_QTWIDGETS_RASTERPROPERTIESDIALOG_H
#define GPLATES_QTWIDGETS_RASTERPROPERTIESDIALOG_H

#include <QDialog>

#include "ui_RasterPropertiesDialogUi.h"

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class EditAffineTransformGeoreferencingWidget;
	class FriendlyLineEdit;
	class InformationDialog;

	class RasterPropertiesDialog :
			public QDialog,
			protected Ui_RasterPropertiesDialog
	{
		Q_OBJECT

	public:

		explicit
		RasterPropertiesDialog(
				GPlatesPresentation::ViewState *view_state,
				QWidget *parent_ = NULL);

		void
		populate_from_data();

	private Q_SLOTS:

		void
		handle_colour_map_lineedit_editing_finished();

		void
		handle_use_default_colour_map_button_clicked();

		void
		handle_open_cpt_button_clicked();

		void
		handle_extent_help_button_clicked();

		void
		handle_properties_help_button_clicked();

		void
		handle_colour_map_help_button_clicked();

		void
		handle_main_buttonbox_clicked(
				QAbstractButton *button);

	private:

		void
		make_signal_slot_connections();

		void
		enable_all_groupboxes(
				bool enabled);

		void
		set_raster_colour_map_filename(
				const QString &filename);

		enum HelpContext
		{
			PROPERTIES,
			EXTENT,
			COLOUR_MAP
		};

		void
		show_help_dialog(
				HelpContext context);

		// FIXME: Remove after rasters are moved out of ViewState.
		GPlatesPresentation::ViewState *d_view_state;

		// Memory managed by Qt.
		EditAffineTransformGeoreferencingWidget *d_georeferencing_widget;
		FriendlyLineEdit *d_colour_map_lineedit;
		InformationDialog *d_help_dialog;
	};
}

#endif // GPLATES_QTWIDGETS_RASTERPROPERTIESDIALOG_H
