/* $Id$ */

/**
 * \file
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QPushButton>
#include <QItemSelectionModel>
#include <QApplication>
#include <QClipboard>

#include "app-logic/ApplicationState.h"
#include "app-logic/LogModel.h"

#include "global/config.h"  // GPLATES_PUBLIC_RELEASE

#include "gui/LogFilterModel.h"

#include "LogDialog.h"



GPlatesQtWidgets::LogDialog::LogDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *_parent):
	GPlatesDialog(_parent, Qt::Window),
	d_filter_timeout(new QTimer(this))
{
	setupUi(this);
	
	// For public releases, switch the Debug checkbox off. Users can still see debug messages
	// if they really must, but don't have to get swamped with them by default.
#if defined(GPLATES_PUBLIC_RELEASE)  // Flag defined by CMake build system (in "global/config.h").

	checkbox_show_debug->setChecked(false);
	
#endif

	// Create a LogFilterModel to filter the app-logic LogModel for us.
	d_log_filter_model_ptr = new GPlatesGui::LogFilterModel(this);
	d_log_filter_model_ptr->setDynamicSortFilter(true);
	d_log_filter_model_ptr->setSourceModel(&app_state.get_log_model());
	
	// Scroll to bottom whenever the source model gets new rows.
	connect(&app_state.get_log_model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
			listview_log, SLOT(scrollToBottom()));
	
	// When the user changes the filtering check-boxes, update the view-filter.
	connect(checkbox_show_debug, SIGNAL(stateChanged(int)), this, SLOT(handle_filter_changed()));
	connect(checkbox_show_warning, SIGNAL(stateChanged(int)), this, SLOT(handle_filter_changed()));
	connect(checkbox_show_critical, SIGNAL(stateChanged(int)), this, SLOT(handle_filter_changed()));
	
	// Similarly, the user can type in the Filter line edit to restrict the view based on a full text search.
	connect(lineedit_filter, SIGNAL(textChanged(QString)), this, SLOT(handle_filter_typing()));
	connect(lineedit_filter, SIGNAL(returnPressed()), this, SLOT(handle_filter_changed()));
	connect(d_filter_timeout, SIGNAL(timeout()), this, SLOT(handle_filter_changed()));
	d_filter_timeout->setSingleShot(true);
	
	// Close button should not be default action for "Enter", since it seems to steal it away from
	// the filter lineedit.
	buttonbox->button(QDialogButtonBox::Close)->setAutoDefault(false);
	buttonbox->button(QDialogButtonBox::Close)->setDefault(false);
		
	// Connect the View to the LogFilterModel.
	listview_log->setModel(d_log_filter_model_ptr);
	
	// The Copy to clipboard button will be available if there is a selection.
	connect(listview_log->selectionModel(), 		// Must come after ->setModel()
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this,
			SLOT(handle_selection_changed()));
	connect(button_copy_to_clipboard, SIGNAL(clicked()), this, SLOT(copy_selection_to_clipboard()));

	// Ensure everything is in sync.
	handle_filter_changed();
}


void
GPlatesQtWidgets::LogDialog::copy_selection_to_clipboard()
{
	QModelIndexList indexes = listview_log->selectionModel()->selectedIndexes();
	
	QString text;
	Q_FOREACH(QModelIndex idx, indexes) {
		text.append(d_log_filter_model_ptr->data(idx).toString());
		text.append("\n");
	}
	
	QApplication::clipboard()->setText(text);
}


void
GPlatesQtWidgets::LogDialog::handle_filter_typing()
{
	// Timer will be started or re-started during typing.
	d_filter_timeout->start(800);
}

void
GPlatesQtWidgets::LogDialog::handle_filter_changed()
{
	if (d_log_filter_model_ptr) {
		d_log_filter_model_ptr->set_filter(
				lineedit_filter->text(),
				checkbox_show_debug->isChecked(),
				checkbox_show_warning->isChecked(),
				checkbox_show_critical->isChecked());
	}
}


void
GPlatesQtWidgets::LogDialog::handle_selection_changed()
{
	button_copy_to_clipboard->setEnabled(listview_log->selectionModel()->hasSelection());
}


