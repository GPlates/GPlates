/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_UNAVAILABLEGUISYSTEMEXCEPTION_H_
#define _GPLATES_GUI_UNAVAILABLEGUISYSTEMEXCEPTION_H_

#include "GuiException.h"

namespace GPlatesGui
{
	/**
	 * This exception is thrown when the code attempts to set the
	 * GuiSystem to be used (which would thus directly control which
	 * WindowFactory would be created) when the WindowFactory has
	 * already been instantiated.
	 */
	class WindowFactoryAlreadyInstantiatedException
		: public GuiException
	{
		public:
			WindowFactoryAlreadyInstantiatedException() {  }

			virtual
			~WindowFactoryAlreadyInstantiatedException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "WindowFactoryAlreadyInstantiatedException";
			}

			virtual std::string
			Message() const { return ""; }
	};
}

#endif  // _GPLATES_GUI_UNAVAILABLEGUISYSTEMEXCEPTION_H_
