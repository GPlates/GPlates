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

#include <boost/noncopyable.hpp>

#include "app-logic/ApplicationState.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ViewportWindow.h"


namespace GPlatesPresentation
{
	/**
	 * Stores the application state, the view state and ViewportWindow.
	 *
	 * This is exposed in Python as the "Instance" class.
	 */
	class Application :
			private boost::noncopyable
	{
	public:

		static
		Application*
		instance()
		{
			static Application* inst = new Application();
			return inst;
		}

		GPlatesAppLogic::ApplicationState &
		get_application_state()
		{
			return d_application_state;
		}

		GPlatesPresentation::ViewState &
		get_view_state()
		{
			return d_view_state;
		}

		GPlatesQtWidgets::ViewportWindow &
		get_viewport_window()
		{
			return d_viewport_window;
		}

	private:
		Application();
		Application(const Application&);
		GPlatesAppLogic::ApplicationState d_application_state;
		GPlatesPresentation::ViewState d_view_state;
		GPlatesQtWidgets::ViewportWindow d_viewport_window;
	};
}

#endif  // GPLATES_PRESENTATION_APPLICATION_H
