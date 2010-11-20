/* $Id: PanMap.h 8194 2010-04-26 16:24:42Z rwatson $ */

/**
 * \file 
 * $Revision: 8194 $
 * $Date: 2010-04-26 18:24:42 +0200 (ma, 26 apr 2010) $ 
 * 
 * Copyright (C)  2010 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_SMALLCIRCLEMANAGER_H
#define GPLATES_QTWIDGETS_SMALLCIRCLEMANAGER_H

#include <QDialog>

#include "maths/SmallCircle.h"
#include "SmallCircleManagerUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
}

namespace GPlatesQtWidgets
{



	class CreateSmallCircleDialog;

	class SmallCircleManager:
		public QDialog,
		protected Ui_SmallCircleManager
	{



		Q_OBJECT
	public:

		enum {
			CENTRE_COLUMN = 0,
			RADIUS_COLUMN,
			NUM_COLUMNS
		};

		SmallCircleManager(
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
			GPlatesAppLogic::ApplicationState &application_state,
			QWidget *parent_ = NULL);

		void
		add_circle(
			const GPlatesMaths::SmallCircle &small_circle);

	private slots:

		void
		handle_add();

		void
		handle_remove();

		void
		handle_circle_added();

		void
		handle_remove_all();

		void
		handle_item_selection_changed();

	private:

		void
		update_buttons();

		GPlatesViewOperations::RenderedGeometryLayer *d_small_circle_layer;

		CreateSmallCircleDialog *d_create_small_circle_dialog_ptr;

		std::vector<GPlatesMaths::SmallCircle> d_small_circle_collection;

	};
}

#endif // GPLATES_QTWIDGETS_SMALLCIRCLEMANAGER_H