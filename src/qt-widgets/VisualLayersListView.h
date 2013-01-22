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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H
#define GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H

#include <QAbstractItemModel>
#include <QListView>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class VisualLayersProxy;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declaration.
	class ViewportWindow;

	class VisualLayersListView :
			public QListView
	{
		Q_OBJECT

	public:

		VisualLayersListView(
				GPlatesGui::VisualLayersProxy &visual_layers,
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_ = NULL);

		virtual
		void
		dragEnterEvent(
				QDragEnterEvent *event_);

		virtual
		void
		dropEvent(
				QDropEvent *event_);

	protected slots:

		virtual
		void
		rowsInserted(
				const QModelIndex &parent_,
				int start,
				int end);

	private slots:

		void
		handle_begin_add_or_remove_layers();

		void
		handle_end_add_or_remove_layers();

	private:

		/**
		 * Opens the persistent editor for entries in the list from @a begin_row up
		 * to the entry before @a end_row (i.e. half-open range).
		 */
		void
		open_persistent_editors(
				int begin_row,
				int end_row);

		/**
		 * Same as @a open_persistent_editors but closes editors.
		 */
		void
		close_persistent_editors(
				int begin_row,
				int end_row);

		void
		make_signal_slot_connections();


		GPlatesGui::VisualLayersProxy &d_visual_layers;
		QAbstractItemModel *d_list_model;

	};

}


#endif	// GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H
