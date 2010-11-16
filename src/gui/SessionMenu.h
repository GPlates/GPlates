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


namespace GPlatesAppLogic
{
	class ApplicationState;
	class SessionManagement;
}


namespace GPlatesGui
{
	class FileIOFeedback;
	class UnsavedChangesTracker;

	/**
	 * This class is responsible for providing the user interface to
	 * GPlatesAppLogic::SessionManagement.
	 */
	class SessionMenu: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		SessionMenu(
				GPlatesAppLogic::ApplicationState &app_state_,
				GPlatesGui::FileIOFeedback &file_io_feedback_,
				GPlatesGui::UnsavedChangesTracker &unsaved_changes_tracker_,
				QObject *parent_ = NULL);

		virtual
		~SessionMenu()
		{  }


		/**
		 * Does Menu initialisation, which must wait until after ViewportWindow
		 * has called setupUi().
		 *
		 * @a menu_ - the QMenu to mess with.
		 */
		void
		init(
				QMenu &menu_);


	public slots:

		void
		open_previous_session();

	private:

		/**
		 * Pointer to the session management, to get session info.
		 */
		GPlatesAppLogic::SessionManagement *d_session_management_ptr;

		/**
		 * Pointer to FileIOFeedback, to initiate change while trapping exceptions.
		 */
		GPlatesGui::FileIOFeedback *d_file_io_feedback_ptr;

		/**
		 * Pointer to UnsavedChangesTracker, in case session loading would clobber
		 * unsaved changes in existing session.
		 */
		GPlatesGui::UnsavedChangesTracker *d_unsaved_changes_tracker_ptr;

		/**
		 * Guarded pointer to the QMenu we are allowed to mess with.
		 */
		QPointer<QMenu> d_menu_ptr;
	};
}


#endif	// GPLATES_GUI_SESSIONMENU_H
