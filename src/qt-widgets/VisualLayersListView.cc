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

#include <QDropEvent>

#include "VisualLayersListView.h"

#include "VisualLayersDelegate.h"

#include "gui/VisualLayersListModel.h"


GPlatesQtWidgets::VisualLayersListView::VisualLayersListView(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QString &open_file_path,
		ReadErrorAccumulationDialog *read_errors_dialog,
		QWidget *parent_) :
	QListView(parent_)
{
	// Customise behaviour.
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setSelectionMode(QAbstractItemView::NoSelection);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setFrameShape(QFrame::NoFrame);

	// Install the model and the delegate.
	GPlatesGui::VisualLayersListModel *list_model =
		new GPlatesGui::VisualLayersListModel(visual_layers, this);
	setModel(list_model);
	VisualLayersDelegate *delegate = new VisualLayersDelegate(
			visual_layers,
			application_state,
			view_state,
			open_file_path,
			read_errors_dialog,
			this);
	setItemDelegate(delegate);

	// Open the persistent editor for all rows in existence at creation.
	open_persistent_editors(0, list_model->rowCount());
}


void
GPlatesQtWidgets::VisualLayersListView::dragEnterEvent(
		QDragEnterEvent *event_)
{
	QListView::dragEnterEvent(event_);

	// Check that we're being dropped on from a target in the same application.
	// source() returns NULL if it's being dragged from another application,
	// including the case where that other application is another instance of GPlates.
	if (!event_->source())
	{
		event_->ignore();
	}
}


void
GPlatesQtWidgets::VisualLayersListView::dropEvent(
		QDropEvent *event_)
{
	QListView::dropEvent(event_);
}


void
GPlatesQtWidgets::VisualLayersListView::rowsInserted(
		const QModelIndex &parent_,
		int start,
		int end)
{
	QListView::rowsInserted(parent_, start, end);

	// Open the persistent editor for the new rows.
	open_persistent_editors(start, end + 1);
}


void
GPlatesQtWidgets::VisualLayersListView::open_persistent_editors(
		int begin_row,
		int end_row)
{
	const QAbstractItemModel *list_model = model();
	for (; begin_row != end_row; ++begin_row)
	{
		openPersistentEditor(list_model->index(begin_row, 0));
	}
}

