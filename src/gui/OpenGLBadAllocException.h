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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_OPENGLBADALLOCEXCEPTION_H_
#define _GPLATES_GUI_OPENGLBADALLOCEXCEPTION_H_

#include "GuiException.h"

namespace GPlatesGui
{
	/**
	 * The Exception thrown by the OpenGL wrappers when OpenGL is unable
	 * to allocate memory for an object.
	 */
	class OpenGLBadAllocException
		: public GuiException
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * in which the problem occurs.
			 */
			OpenGLBadAllocException(const char *msg)
				: _msg(msg) {  }

			virtual
			~OpenGLBadAllocException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "OpenGLBadAllocException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_GUI_OPENGLBADALLOCEXCEPTION_H_
