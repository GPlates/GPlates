/* $Id$ */

/**
 * \file Responsible for managing instances of GPlatesQtWidgets::GPlatesDialog in the application.
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
 
#ifndef GPLATES_GUI_DIALOGS_H
#define GPLATES_GUI_DIALOGS_H

#include <QPointer>
#include "boost/noncopyable.hpp"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GPlatesDialog;
	class ViewportWindow;
	
	////////////////////////////////////////////////
	// Forward declarations for our dialogs.
	////////////////////////////////////////////////
	class LogDialog;
	class PreferencesDialog;
}

namespace GPlatesGui
{
	/**
	 * Class responsible for managing instances of GPlatesDialog in the application.
	 * 
	 * Major dialogs and handy floating window-like things that typically hang off of ViewportWindow
	 * are to be managed here to avoid further cluttering up the ViewportWindow class itself.
	 *
	 * All such instances are of our QDialog subclass, GPlatesDialog, so that we have a few helper
	 * methods available. All are parented to ViewportWindow itself, so that Qt knows how the hierarchy
	 * of windows and dialogs should be logically arranged. However, methods for accessing those
	 * dialogs should be kept here.
	 *
	 * IMPORTANT NOTE: The exact flow of this class isn't quite finalised yet, so don't refactor
	 *                 everything in ViewportWindow just yet. Specifically, I want a neat way to
	 *                 ensure that menu action connections can be done without having to #include
	 *                 the entire dialog in ViewportWindow.cc.
	 */
	class Dialogs :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
	public:
	
		/**
		 * Much like the ApplicationState-members, GPlatesGui::Dialogs should be instantiated and
		 * kept somewhere nice. It is a QObject so that we can use signals and slots, and will be
		 * noncopyable.
		 */
		explicit
		Dialogs(
				GPlatesAppLogic::ApplicationState &_application_state,
				GPlatesPresentation::ViewState &_view_state,
				GPlatesQtWidgets::ViewportWindow &_viewport_window,
				QObject *_parent);
		
		virtual
		~Dialogs()
		{  }

		////////////////////////////////////////////////////////////////////////
		// Here are all the accessors for dialogs managed by this class.
		////////////////////////////////////////////////////////////////////////

		GPlatesQtWidgets::LogDialog &
		log_dialog();
		

		GPlatesQtWidgets::PreferencesDialog &
		preferences_dialog();


		////////////////////////////////////////////////////////////////////////

	public slots:

		////////////////////////////////////////////////////////////////////////
		// And here are wrappers around various_dialogs().pop_up() so that
		// those dialogs which support it can be lazy-loaded after the user
		// triggers their appropriate menu item.
		//
		// Not recommended for anything that has tight integration with the
		// rest of GPlates, as it may cause Heisenbugs that only occur before
		// or after certain dialogs are shown for the first time - such as
		// one involving the Configure Animation Dialog's slider.
		////////////////////////////////////////////////////////////////////////

		void
		lazy_pop_up_log_dialog();


		////////////////////////////////////////////////////////////////////////

		/**
		 * Closes any QDialog instances parented to ViewportWindow.
		 */
		void
		close_all_dialogs();

	private:
	
		/**
		 * Convenience method to get at ApplicationState.
		 */
		GPlatesAppLogic::ApplicationState &
		application_state() const;

		/**
		 * Convenience method to get at ViewState.
		 */
		GPlatesPresentation::ViewState &
		view_state() const;
		
		/**
		 * Convenience method to get at ViewportWindow.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window() const;


		/**
		 * We keep guarded pointers to major GPlates classes to help with dialog construction.
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_application_state_ptr;
		QPointer<GPlatesPresentation::ViewState> d_view_state_ptr;
		QPointer<GPlatesQtWidgets::ViewportWindow> d_viewport_window_ptr;
	};
}


#endif //GPLATES_GUI_DIALOGS_H
