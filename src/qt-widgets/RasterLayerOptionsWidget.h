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
 
#ifndef GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H

#include <vector>
#include <boost/optional.hpp>
#include <QWidget>
#include <QString>

#include "RasterLayerOptionsWidgetUi.h"

#include "app-logic/Layer.h"


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
	// Forward declaration.
	class FriendlyLineEdit;
	class ReadErrorAccumulationDialog;

	/**
	 * RasterLayerOptionsWidget is used to show additional options for raster and
	 * age grid layers in the visual layers widget.
	 */
	class RasterLayerOptionsWidget :
			public QWidget,
			protected Ui_RasterLayerOptionsWidget
	{
		Q_OBJECT
		
	public:

		explicit
		RasterLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QString &open_file_path,
				ReadErrorAccumulationDialog *read_errors_dialog,
				QWidget *parent_ = NULL);

		void
		set_data(
				const GPlatesAppLogic::Layer &layer);

	private slots:

		void
		handle_band_combobox_activated(
				const QString &text);

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

	private:

		void
		make_signal_slot_connections();

		static const QString PALETTE_FILENAME_BLANK_TEXT;

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		QString &d_open_file_path;
		ReadErrorAccumulationDialog *d_read_errors_dialog;

		FriendlyLineEdit *d_palette_filename_lineedit;

		/**
		 * The layer that we're currently displaying.
		 */
		GPlatesAppLogic::Layer d_current_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H
