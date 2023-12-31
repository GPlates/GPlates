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
 
#ifndef GPLATES_GUI_SESSIONMENU_H
#define GPLATES_GUI_SESSIONMENU_H

#include <QObject>
#include <QPointer>
#include <QMenu>
#include <QActionGroup>
#include <QList>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class SessionManagement;
	class ViewState;
}

namespace GPlatesGui
{
	class FileIOFeedback;

	/**
	 * This class is responsible for providing the user interface to SessionManagement.
	 */
	class SessionMenu: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		SessionMenu(
				GPlatesAppLogic::ApplicationState &app_state_,
				GPlatesPresentation::ViewState &view_state_,
				GPlatesGui::FileIOFeedback &file_io_feedback_,
				QObject *parent_ = NULL);

		virtual
		~SessionMenu()
		{  }


		/**
		 * Does Menu Action initialisation, which must wait until after ViewportWindow
		 * has called setupUi().
		 *
		 * @a menu_ - the QMenu to mess with.
		 */
		void
		init(
				QMenu &menu_);


	public Q_SLOTS:

		/**
		 * Relabels and shows/hides appropriate Menu Actions to match the current
		 * Recent Sessions List as returned by GPlatesAppLogic::SessionManagement.
		 */
		void
		regenerate_menu();


		void
		open_previous_session(
				int session_slot_to_load = 0);

	private Q_SLOTS:

		void
		handle_action_triggered(
				QAction *act);

	private:

		/**
		 * Pointer to the session management, to get session info.
		 */
		GPlatesPresentation::SessionManagement *d_session_management_ptr;

		/**
		 * Pointer to FileIOFeedback, to initiate change while trapping exceptions.
		 */
		GPlatesGui::FileIOFeedback *d_file_io_feedback_ptr;

		/**
		 * Guarded pointer to the QMenu we are allowed to mess with.
		 */
		QPointer<QMenu> d_menu_ptr;


		/**
		 * The "No sessions to load" placeholder Action.
		 */
		QPointer<QAction> d_no_sessions_action;
		
		/**
		 * One QAction for each potential session to restore, in order.
		 */
		QList<QPointer<QAction> > d_recent_session_actions;
		QActionGroup d_recent_session_action_group;
	};
}


#endif	// GPLATES_GUI_SESSIONMENU_H
