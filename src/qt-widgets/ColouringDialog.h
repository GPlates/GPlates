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
 
#ifndef GPLATES_QTWIDGETS_COLOURINGDIALOG_H
#define GPLATES_QTWIDGETS_COLOURINGDIALOG_H

#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "ColouringDialogUi.h"
#include "ColourSchemeInfo.h"
#include "GlobeCanvas.h"
#include "MapView.h"
#include "MapCanvas.h"

#include "gui/ColourScheme.h"
#include "model/FeatureStoreRootHandle.h"

#include "presentation/ViewState.h" // FIXME: remove later
#include <QLabel>

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class PaletteSelectionWidget;

	class ColouringDialog : 
			public QDialog, 
			protected Ui_ColouringDialog 
	{
		Q_OBJECT

	public:

		/**
		 * Constructs a ColouringDialog. Clones @a existing_globe_canvas_ptr for the
		 * previews.
		 */
		ColouringDialog(
			GPlatesPresentation::ViewState &view_state,
			GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
			QWidget* parent_ = NULL);

	private slots:

		void
		close_button_pressed();

		void
		handle_globe_repainted(bool);

	private:

		typedef std::map<QString, std::vector<ColourSchemeInfo> > colour_schemes_map_type;

		void
		make_signal_slot_connections();

		void
		create_colour_schemes();

		void
		populate_feature_collections();

		GPlatesPresentation::ViewState &d_view_state;

		PaletteSelectionWidget *d_palette_selection_widget_ptr;

		/**
		 * Stores the colour schemes loaded and selectable in the colouring dialog.
		 */
		colour_schemes_map_type d_colour_schemes;

		/**
		 * A weak-ref to the feature store root, so we can find out about new feature collections.
		 */
		GPlatesModel::FeatureStoreRootHandle::const_weak_ref d_feature_store_root;

		// For testing
		GlobeAndMapWidget *d_globe;
		QLabel *d_label;
		
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURINGDIALOG_H
