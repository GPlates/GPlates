/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef _GPLATES_GUI_GUIEXCEPTION_H_
#define _GPLATES_GUI_GUIEXCEPTION_H_

#include "global/Exception.h"

namespace GPlatesGui
{
	/**
	 * The (pure virtual) base class of all GUI exceptions.
	 */
	class GuiException : public GPlatesGlobal::Exception
	{
		public:
			virtual
			~GuiException() {  }
	};
}

#endif  // _GPLATES_GUI_GUIEXCEPTION_H_
