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

#include "gui/FileIOFeedback.h"

#include "presentation/InternalSession.h"
#include "presentation/SessionManagement.h"
#include "presentation/ViewState.h"


namespace
{
	QString
	create_tooltip_from_session(
			const GPlatesPresentation::Session &session)
	{
		QStringList files = session.get_loaded_files();
		return files.join("\n");
	}

	QString
	create_statustip_from_session(
			const GPlatesPresentation::Session &session)
	{
		QStringList files = session.get_loaded_files();
		return files.join(", ");
	}
}


GPlatesGui::SessionMenu::SessionMenu(
		GPlatesAppLogic::ApplicationState &app_state_,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesGui::FileIOFeedback &file_io_feedback_,
		QObject *parent_):
	QObject(parent_),
	d_session_management_ptr(&(view_state_.get_session_management())),
	d_file_io_feedback_ptr(&file_io_feedback_),
	d_no_sessions_action(new QAction(tr("<No sessions to load>"), d_menu_ptr)),
	d_recent_session_action_group(NULL)
{  }


void
GPlatesGui::SessionMenu::init(
		QMenu &menu_)
{
	// Remember which menu we're supposed to be modifying.
	d_menu_ptr = &menu_;
	
	// When the SessionManagement list of sessions gets updated, update the menu.
	connect(d_session_management_ptr, SIGNAL(session_list_updated()),
			this, SLOT(regenerate_menu()));
	
	// Configure an Action Group so we can listen to just one signal from the whole lot.
	d_recent_session_action_group.setExclusive(false);
	connect(&d_recent_session_action_group, SIGNAL(triggered(QAction *)),
			this, SLOT(handle_action_triggered(QAction *)));

	// The "placeholder" action visible only when there's nothing else to load.
	d_no_sessions_action->setMenuRole(QAction::NoRole);
	d_no_sessions_action->setDisabled(true);
	d_menu_ptr->addAction(d_no_sessions_action);
	
	// All potentially-visible actions for each possible "slot" a session could occupy.
	// Note that we don't add/remove Actions that match each Session, as the book-keeping
	// would get very ugly. Instead, we use a fixed number of Actions that can be hidden
	// or renamed as necessary, and we don't want to manage an infinite number of signal
	// and slot connections.
	for (int i = 0; i < 24; ++i) {
		QAction *act = new QAction(tr("Session %1").arg(i), d_menu_ptr);
		act->setMenuRole(QAction::NoRole);
		act->setData(QVariant(i));
		d_menu_ptr->addAction(act);
		d_recent_session_actions << act;
		d_recent_session_action_group.addAction(act);
	}
	
	// Populate menu with appropriate labels.
	regenerate_menu();
}


void
GPlatesGui::SessionMenu::regenerate_menu()
{
	QList<GPlatesPresentation::InternalSession::non_null_ptr_to_const_type> recent_sessions =
			d_session_management_ptr->get_recent_session_list();
	
	if (recent_sessions.isEmpty()) {
		// There are no sessions to restore; display one disabled "label" menu
		// item indicating the fact, and hide all other menu items.
		d_no_sessions_action->setVisible(true);
		Q_FOREACH(QAction *act, d_recent_session_actions) {
			act->setVisible(false);
		}

	} else {
		// There are some sessions to restore; hide the disabled "label" menu
		// item, along with any superfluous menu items, and add text to the
		// rest depending on what session is in that 'slot'.
		d_no_sessions_action->setVisible(false);
		for (int i = 0; i < d_recent_session_actions.size(); ++i) {
			QAction *act = d_recent_session_actions.at(i);
			if (i < recent_sessions.size()) {
				const GPlatesPresentation::InternalSession &session = *recent_sessions.at(i);

				// This menu slot corresponds to a Session on the list.
				act->setVisible(true);

				act->setText(session.get_description());

				act->setToolTip(create_tooltip_from_session(session));
				act->setStatusTip(create_statustip_from_session(session));
			} else {
				// This menu slot has no associated Session.
				act->setVisible(false);
			}
		}

	}
}


void
GPlatesGui::SessionMenu::open_previous_session(
		int session_slot_to_load)
{
	d_file_io_feedback_ptr->open_previous_session(session_slot_to_load);
}


void
GPlatesGui::SessionMenu::handle_action_triggered(
		QAction *act)
{
	int session_slot_to_load = act->data().toInt();
	open_previous_session(session_slot_to_load);
}
