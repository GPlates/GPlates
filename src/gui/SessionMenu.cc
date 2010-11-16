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

#include "SessionMenu.h"

#include <QDebug>
#include <QAction>

#include "app-logic/ApplicationState.h"
#include "app-logic/SessionManagement.h"

#include "gui/FileIOFeedback.h"
#include "gui/UnsavedChangesTracker.h"


namespace
{
	QString
	create_tooltip_from_session(
			const GPlatesAppLogic::Session &session)
	{
		QStringList files = QStringList::fromSet(session.loaded_files());
		return files.join("\n");
	}

	QString
	create_statustip_from_session(
			const GPlatesAppLogic::Session &session)
	{
		QStringList files = QStringList::fromSet(session.loaded_files());
		return files.join(", ");
	}
}


GPlatesGui::SessionMenu::SessionMenu(
		GPlatesAppLogic::ApplicationState &app_state_,
		GPlatesGui::FileIOFeedback &file_io_feedback_,
		GPlatesGui::UnsavedChangesTracker &unsaved_changes_tracker_,
		QObject *parent_):
	QObject(parent_),
	d_session_management_ptr(&(app_state_.get_session_management())),
	d_file_io_feedback_ptr(&file_io_feedback_),
	d_unsaved_changes_tracker_ptr(&unsaved_changes_tracker_)
{  }


void
GPlatesGui::SessionMenu::init(
		QMenu &menu_)
{
	d_menu_ptr = &menu_;

	// TEMP: One session only.
	QList<GPlatesAppLogic::Session> recent_sessions =
			d_session_management_ptr->get_recent_session_list();
	
	QAction *restore = new QAction(tr("<No sessions to load>"), d_menu_ptr);
	connect(restore, SIGNAL(triggered()),
			this, SLOT(open_previous_session()));

	if (recent_sessions.isEmpty()) {
		restore->setDisabled(true);
	} else {
		restore->setText(recent_sessions.front().description());
		restore->setToolTip(create_tooltip_from_session(recent_sessions.front()));
		restore->setStatusTip(create_statustip_from_session(recent_sessions.front()));
	}

	d_menu_ptr->addAction(restore);
}


void
GPlatesGui::SessionMenu::open_previous_session()
{
	if (d_unsaved_changes_tracker_ptr->replace_session_event_hook()) {
		d_file_io_feedback_ptr->open_previous_session();
	}
}

