/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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


namespace GPlatesPresentation
{
	/**
	 * Stores the application state and the view state.
	 */
	class Application :
			private boost::noncopyable
	{
	public:
		Application();


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


	private:
		GPlatesAppLogic::ApplicationState d_application_state;
		GPlatesPresentation::ViewState d_view_state;
	};
}

#endif // GPLATES_PRESENTATION_APPLICATION_H
