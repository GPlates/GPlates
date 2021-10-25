/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_APPLICATION_H
#define GPLATES_PRESENTATION_APPLICATION_H

#include <boost/optional.hpp>

#include "app-logic/ApplicationState.h"

#include "global/GPlatesAssert.h"

#include "gui/CommandServer.h"
#include "gui/ExternalSyncController.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ViewportWindow.h"

#include "scribe/ScribeExceptions.h"
#include "scribe/Transcribe.h"

#include "utils/Singleton.h"

namespace GPlatesGui
{
	class CommandServer;
}

namespace GPlatesPresentation
{
	/**
	 * Stores the application state, the view state and ViewportWindow.
	 *
	 * This is exposed in Python as the "Instance" class.
	 */
	class Application :
			public GPlatesUtils::Singleton<Application>
	{
		// Note the use of 'PUBLIC' in the macro - this so a single 'Application' instance can be
		// created on the C runtime stack as a means of lifetime control of the singleton -
		// to make sure it gets destroyed when exiting the scope in which the instance lives.
		// While it is in scope it can also be accessed by 'Application::instance()' as normal.
		GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DECL(Application)

	public:

		/**
		 * Returns the state at the application-logic level.
		 *
		 * This is separate from the presentation/view state (and windowing/widget state) and
		 * knows nothing of their existence.
		 */
		GPlatesAppLogic::ApplicationState &
		get_application_state()
		{
			return d_application_state;
		}


		/**
		 * Returns the state at the presentation/view level.
		 *
		 * This is separate from the windowing/widget state and while it supports the
		 * windowing/widget state it knows nothing of its existence.
		 */
		GPlatesPresentation::ViewState &
		get_view_state()
		{
			return d_view_state;
		}


		/**
		 * Returns the state at the windowing/widget level.
		 *
		 * This is the main window which is currently the top of the widget hierarchy.
		 * All other widgets/dialogs/etc can be obtained directly or indirectly from it.
		 */
		GPlatesQtWidgets::ViewportWindow &
		get_main_window()
		{
			return d_main_window;
		}

	
		/**
		 * Enable communication between GPlates and other (external) applications.
		 *
		 * We need to control this via the main window for situations where GPlates is
		 * launched remotely and acts as the "slave" application.
		 * This would also disable the ability to open the external-sync-dialog from GPlates.
		 */
		void
		enable_syncing_with_external_applications(
				bool gplates_is_master = false);


		/**
		 * Sets the current reconstruction time with the presentation-level animation controller.
		 *
		 * The animation controller in turn sets the reconstruction time on the application-logic state
		 * and also manages signals used at the widget/presentation level.
		 */
		void
		set_reconstruction_time(
				const double &reconstruction_time);

	private:

		/**
		 * Perform any initialisation that doesn't necessarily belong in the constructors of
		 * @a ViewportWindow, @a ViewState or @a ApplicationState.
		 *
		 * This includes connecting signal/slots of view/application state objects to widgets
		 * obtained directly, or indirectly, from @a ViewportWindow.
		 * This is because @a ViewportWindow should really just be the container of a menubar,
		 * the reconstruction view widget, the canvas tools dock widget, the search results dock widget
		 * and various dialogs.
		 * Those objects, in turn, can then be queried for their sub-objects.
		 * And this initialisation function is the place to make connections to those sub-objects.
		 *
		 * NOTE: Code from the constructor of @a ViewportWindow was moved here.
		 */
		void
		initialise();


		GPlatesAppLogic::ApplicationState d_application_state;
		GPlatesPresentation::ViewState d_view_state;
		GPlatesQtWidgets::ViewportWindow d_main_window;
		GPlatesGui::CommandServer d_cmd_server;

		/**
		 * Controller for external communication.
		 */
		boost::optional<GPlatesGui::ExternalSyncController> d_external_sync_controller;

	private: // Transcribing...

		/**
		 * Saves or restores a session.
		 *
		 * Whether a session is to be restored or saved depends on the state of the specified Scribe.
		 * The same code path handles both save and restore (to help ensure they don't get out-of-sync).
		 *
		 * This is the main entry point for saving and restoring sessions in GPlates because
		 * this class (Application) encapsulates all GPlates state.
		 */
		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<Application> &application)
		{
			// Shouldn't construct object - always transcribe existing object.
			GPlatesGlobal::Assert<GPlatesScribe::Exceptions::ConstructNotAllowed>(
					false,
					GPLATES_ASSERTION_SOURCE,
					typeid(Application));

			// Shouldn't be able to get here - keep compiler happy.
			return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
		}

		// Only the scribe system should be able to transcribe.
		friend class GPlatesScribe::Access;
	};

	inline
	double
	current_time()
	{
		return Application::instance().get_application_state().get_current_reconstruction_time();
	}
}

#endif  // GPLATES_PRESENTATION_APPLICATION_H
