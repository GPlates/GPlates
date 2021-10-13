/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H
#define GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QTreeWidget>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <boost/scoped_ptr.hpp>

#include "DigitisationWidgetUi.h"
#include "LatLonCoordinatesTable.h"
#include "TaskPanelWidget.h"
#include "ViewportWindow.h"

#include "maths/GeometryOnSphere.h"
#include "model/ModelInterface.h"


namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;
}

namespace GPlatesQtWidgets
{
	// Forward declaration to avoid the GUI header spaghetti as much as possible.
	class ExportCoordinatesDialog;
	class CreateFeatureDialog;
	//class ViewportWindow;
	class LatLonCoordinatesTable;

	class DigitisationWidget :
			public TaskPanelWidget, 
			protected Ui_DigitisationWidget
	{
		Q_OBJECT

	public:

		explicit
		DigitisationWidget(
				GPlatesViewOperations::GeometryBuilder &new_geometry_builder,
				GPlatesPresentation::ViewState &view_state_,
				ViewportWindow &viewport_window_,
				QAction *clear_action,
				QAction *undo_action,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
				QWidget *parent_ = NULL);

		~DigitisationWidget();

		/**
		 * Accessor for the Export Coordinates Dialog, for signal/slot connections etc.
		 */
		ExportCoordinatesDialog &
		get_export_coordinates_dialog()
		{
			return *d_export_coordinates_dialog;
		}

		/**
		 * Accessor for the Create Feature Dialog, for signal/slot connections etc.
		 */
		CreateFeatureDialog &
		get_create_feature_dialog()
		{
			return *d_create_feature_dialog;
		}

		void
		reload_coordinates_table_if_necessary()
		{
			d_lat_lon_coordinates_table->reload_if_necessary();
		}

		virtual
		void
		handle_activation();

		virtual
		QString
		get_clear_action_text() const;

		virtual
		bool
		clear_action_enabled() const;

		virtual
		void
		handle_clear_action_triggered();

	private slots:

		/**
		 * The slot that gets called when the user clicks "Create".
		 */
		void
		handle_create();

		/**
		 * Feeds the ExportCoordinatesDialog a GeometryOnSphere, and
		 * then displays it.
		 */
		void
		handle_export();

		/**
		 * Feeds the ExportCoordinatesDialog a GeometryOnSphere, and
		 * then displays it.
		 */
		void
		handle_use_in_wfs();

		/**
		 * The slot that gets called when the geometry inside the geometry
		 * builder is changed.
		 */
		void
		handle_geometry_changed();

	private:

		// The almighty Viewport Window , holder of all dialogs!
		ViewportWindow *d_viewport_window;
		
		/**
		 * The dialog the user sees when they hit the Export button.
		 * Memory managed by Qt.
		 */
		ExportCoordinatesDialog *d_export_coordinates_dialog;

		/**
		 * The dialog the user sees when they hit the Create button.
		 * Memory managed by Qt.
		 */
		CreateFeatureDialog *d_create_feature_dialog;

		/**
		 * The new geometry @a GeometryBuilder we use when we need to create
		 * new feature geometry.
		 */
		GPlatesViewOperations::GeometryBuilder *d_new_geom_builder;

		/**
		 * Used by clear geometry undo operation.
		 */
		GPlatesGui::ChooseCanvasTool *d_choose_canvas_tool;

		/**
		 * A wrapper around coordinates table that listens to a GeometryBuilder
		 * and fills in the table accordingly.
		 */
		boost::scoped_ptr<LatLonCoordinatesTable> d_lat_lon_coordinates_table;

		void
		make_signal_slot_connections();

		QTreeWidget *
		coordinates_table()
		{
			return treewidget_coordinates;
		}
	};
}

#endif  // GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H

