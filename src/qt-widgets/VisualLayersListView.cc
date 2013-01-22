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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/VisualLayersListModel.h"
#include "gui/VisualLayersProxy.h"


GPlatesQtWidgets::VisualLayersListView::VisualLayersListView(
		GPlatesGui::VisualLayersProxy &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QListView(parent_),
	d_visual_layers(visual_layers),
	d_list_model(NULL)
{
	// Customise behaviour.
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setSelectionMode(QAbstractItemView::NoSelection);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setFrameShape(QFrame::NoFrame);
	setFocusPolicy(Qt::NoFocus);

	// Install the model and the delegate.
	d_list_model = new GPlatesGui::VisualLayersListModel(visual_layers, this); // Qt managed memory
	setModel(d_list_model);
	VisualLayersDelegate *delegate = new VisualLayersDelegate(
			visual_layers,
			application_state,
			view_state,
			viewport_window,
			this);
	setItemDelegate(delegate);

	make_signal_slot_connections();

	// Open the persistent editor for all rows in existence at creation.
	open_persistent_editors(0, d_list_model->rowCount());
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

	// If we are currently connected to a model then open the persistent editors.
	// Otherwise we will open the persistent editors when we reconnect to the model since
	// that's more efficient when adding/removing multiple layers.
	// This is really just to catch the case where a layer was added without the
	// 'begin_add_or_remove_layers' / 'end_add_or_remove_layers' VisualLayers signals getting emitted.
	if (model())
	{
		// Open the persistent editor for the new rows.
		open_persistent_editors(start, end + 1);
	}
}


void
GPlatesQtWidgets::VisualLayersListView::handle_begin_add_or_remove_layers()
{
	// Close the persistent editors for all rows currently in existence.
	close_persistent_editors(0, d_list_model->rowCount());

	// Disconnect from the model to prevent the view from being updated by it.
	// The model will be reconnected when 'handle_add_add_or_remove_layers()' is called.
	//
	// This is an optimisation that *dramatically* speeds up the addition (or removal) of layers
	// during file loading (or session restore).
	// The gain is not so noticeable for a few layers but for a few tens of layers the difference
	// is very noticeable.
	setModel(NULL);
}


void
GPlatesQtWidgets::VisualLayersListView::handle_end_add_or_remove_layers()
{
	// Now that the model has been updated we reconnect to it.
	// The view will then update itself from the model.
	//
	// This is an optimisation that *dramatically* speeds up the addition (or removal) of layers
	// during file loading (or session restore).
	// The gain is not so noticeable for a few layers but for a few tens of layers the difference
	// is very noticeable.
	setModel(d_list_model);

	// Open the persistent editors for all rows currently in existence.
	open_persistent_editors(0, d_list_model->rowCount());
}


void
GPlatesQtWidgets::VisualLayersListView::open_persistent_editors(
		int begin_row,
		int end_row)
{
	const QAbstractItemModel *list_model = model();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			list_model,
			GPLATES_ASSERTION_SOURCE);

	for (; begin_row != end_row; ++begin_row)
	{
		openPersistentEditor(list_model->index(begin_row, 0));
	}
}


void
GPlatesQtWidgets::VisualLayersListView::close_persistent_editors(
		int begin_row,
		int end_row)
{
	const QAbstractItemModel *list_model = model();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			list_model,
			GPLATES_ASSERTION_SOURCE);

	for (; begin_row != end_row; ++begin_row)
	{
		closePersistentEditor(list_model->index(begin_row, 0));
	}
}


void
GPlatesQtWidgets::VisualLayersListView::make_signal_slot_connections()
{
// 	QObject::connect(
// 			&d_visual_layers,
// 			SIGNAL(begin_add_or_remove_layers()),
// 			this,
// 			SLOT(handle_begin_add_or_remove_layers()));
// 	QObject::connect(
// 			&d_visual_layers,
// 			SIGNAL(end_add_or_remove_layers()),
// 			this,
// 			SLOT(handle_end_add_or_remove_layers()));
}
